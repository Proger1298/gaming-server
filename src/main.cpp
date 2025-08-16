#include "sdk.h"

#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <iostream>
#include <filesystem>
#include <thread>
#include <memory>

#include "ticker.h"
#include "programm_options.h"
#include "json_loader.h"
#include "api_request_handler.h"
#include "file_request_handler.h"
#include "request_handler.h"
#include "logger.h"
#include "serialization_listener.h"

using namespace std::literals;
using namespace logger;
namespace net = boost::asio;
namespace sys = boost::system;
namespace fs = std::filesystem;

namespace {

constexpr const char GAME_DB_URL[]{"GAME_DB_URL"};

application::AppConfig GetConfigFromEnv() {
    application::AppConfig config;
    if (const auto* url = std::getenv(GAME_DB_URL)) {
        config.db_url = url;
    } else {
        throw std::runtime_error(GAME_DB_URL + " environment variable not found"s);
    }
    return config;
}

// Запускает функцию fn на n потоках, включая текущий
template <typename Fn>
void RunWorkers(unsigned n, const Fn& fn) {
    n = std::max(1u, n);
    std::vector<std::jthread> workers;
    workers.reserve(n - 1);
    // Запускаем n-1 рабочих потоков, выполняющих функцию fn
    while (--n) {
        workers.emplace_back(fn);
    }
    fn();
}

}  // namespace

int main(int argc, const char* argv[]) {
    InitLogger();
    auto args = programm_options::ParseCommandLine(argc, argv);

    if (!args) {
        std::cerr << "Usage: ./game_server [--tick-period <time-in-ms>] --config-file <config-path> "
                  << "--www-root <static-files-dir> --randomize-spawn-points=<1/0>"
                  << "[--state-file <state-file>] [--save-state-period <time-in-ms>]" 
                  << std::endl;
        return EXIT_FAILURE;
    }
    try {
        // 1. Загружаем карту из файла и построить модель игры
        model::Game game = json_loader::LoadGame(args->config_file, args->randomize_spawn_points);
        fs::path static_files_dir{ args->www_root };

        // 2. Инициализируем io_context
        const unsigned num_threads = std::thread::hardware_concurrency();
        net::io_context ioc(num_threads);

        // 3. Создаем приложение
        application::Application app{ std::move(game), application::AppConfig{} };
        // application::Application app{ std::move(game), GetConfigFromEnv() }; - ДЛЯ ТЕСТИРОВАНИЯ ЛОКАЛЬНО, НЕ В КОНТЕЙНЕРЕ

        // 4. Добавляем слушатель для сериализации, если указан файл состояния
        std::shared_ptr<serialization::SerializingListener> listener;
        if (!args->state_file.empty()) {
            auto save_period = args->save_state_period > 0 
                ? std::chrono::milliseconds(args->save_state_period)
                : std::chrono::milliseconds::max();

            listener = std::make_shared<serialization::SerializingListener>(
                app, 
                args->state_file, 
                save_period
            );

            if (!listener->TryLoadState()) {
                LOG_WITH_DATA(error, ServerStopedData(EXIT_FAILURE, "Failed to load state"), "server stopped");
                return EXIT_FAILURE;
            }

            app.SetListener(listener);
        }

        // 5. Добавляем асинхронный обработчик сигналов SIGINT и SIGTERM
        net::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&ioc](const sys::error_code& ec, [[maybe_unused]] int signal_number) {
            if (!ec) {
                LOG_WITH_DATA(info, ServerStopedData(0), "server exited"sv);
                ioc.stop();
            }
        });

        // 6.1 strand для выполнения запросов к API
        auto api_strand = net::make_strand(ioc);

        // 6.2 Создаем обработчики
        std::shared_ptr<http_handler::ApiRequestHandler> api_handler{
            http_handler::ApiRequestHandler::Create(
                app,
                api_strand,
                !args->tick_period
            )
        };
        http_handler::FileRequestHandler file_handler{ static_files_dir };

        // 6.3 Создаём обработчик HTTP-запросов и связываем его с API и File обработчиками
        auto handler = std::make_shared<http_handler::RequestHandler>(std::move(api_handler), std::move(file_handler));

        // 7. Запустить обработчик HTTP-запросов, делегируя их обработчику запросов
        const auto address = net::ip::make_address("0.0.0.0");
        constexpr net::ip::port_type port = 8080;
        http_server::ServeHttp(ioc, {address, port}, [&handler](auto&& req, auto&& send) {
            handler->operator()(std::forward<decltype(req)>(req), std::forward<decltype(send)>(send));
        });

        // 8. Настраиваем вызов метода Application::Tick каждые time_delta миллисекунд внутри strand
        if (args->tick_period) {
            auto ticker = std::make_shared<ticker::Ticker>(api_strand, std::chrono::milliseconds(args->tick_period),
                [&app](std::chrono::milliseconds time_delta) { app.Tick(time_delta); }
            );
            ticker->Start();
        }

        // Эта надпись сообщает тестам о том, что сервер запущен и готов обрабатывать запросы
        LOG_WITH_DATA(info, ServerStartedData(address.to_string(), port), "server started"sv);

        // 9. Запускаем обработку асинхронных операций
        RunWorkers(std::max(1u, num_threads), [&ioc] {
            ioc.run();
        });

        // 10. Cохранение при штатном завершении
        if (listener) {
            listener->OnShutdown();
        }

    } catch (const std::exception& ex) {
        LOG_WITH_DATA(error, ServerStopedData(EXIT_FAILURE, ex.what()), "server stopped"sv);
        return EXIT_FAILURE;
    }
}

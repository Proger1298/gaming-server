#include "programm_options.h"

#include <iostream>

namespace programm_options {

[[nodiscard]] std::optional<Args> ParseCommandLine(int argc, const char* const argv[]) {
    namespace po = boost::program_options;

    po::options_description desc{"All options"s};

    Args args;
    desc.add_options()
        // Добавляем опцию --help и её короткую версию -h
        ("help,h", "Usage: ./game_server --tick-period <time-in-ms> --config-file <config-path> --www-root <static-files-dir> --randomize-spawn-points")
        // Опция --tick-period milliseconds задаёт период автоматического обновления игрового состояния в миллисекундах
        ("tick-period,t", po::value(&args.tick_period)->value_name("milliseconds"s), "set tick period")
        // Опция --config-file file задаёт путь к конфигурационному JSON-файлу игры
        ("config-file,c", po::value(&args.config_file)->value_name("file"s), "set config file path")
        // Опция --www-root dir задаёт путь к каталогу со статическими файлами игры
        ("www-root,w", po::value(&args.www_root)->value_name("dir"s), "set static files root")
        // Опция --randomize-spawn-points включает режим, при котором пёс игрока появляется в случайной точке случайно выбранной дороги карты
        ("randomize-spawn-points", po::value(&args.randomize_spawn_points), "spawn dogs at random positions")
        // Опция --state-file file задает путь к файлу сохранения состояния сервера
        ("state-file", po::value(&args.state_file)->value_name("file"s), "set state file path")
        // Опция --save-state-period milliseconds задаёт период автоматического сохранения игрового состояния в миллисекундах
        ("save-state-period", po::value(&args.save_state_period)->value_name("milliseconds"s), "set save state period");

    // variables_map хранит значения опций после разбора
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.contains("help"s)) {
        // Если был указан параметр --help, то выводим справку и возвращаем nullopt
        std::cout << desc;
        return std::nullopt;
    }

    // Проверяем наличие опций src и dst
    if (!vm.contains("config-file"s)) {
        throw std::runtime_error("Config file path has not been specified"s);
    }
    if (!vm.contains("www-root"s)) {
        throw std::runtime_error("Static files root has not been specified"s);
    }

    // С опциями программы всё в порядке, возвращаем структуру args
    return args;
}

} // namespace programm_options
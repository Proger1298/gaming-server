#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <catch2/catch_test_macros.hpp>
#include <boost/json.hpp>
#include <sstream>
#include <filesystem>

#include "../src/model.h"
#include "../src/model_serialization.h"
#include "../src/application.h"
#include "../src/serialization_listener.h"
#include "../src/json_loader.h"

using namespace model;
using namespace application;
using namespace serialization;
using namespace extra_data;
using namespace std::literals;
namespace json = boost::json;

namespace {

using InputArchive = boost::archive::text_iarchive;
using OutputArchive = boost::archive::text_oarchive;

struct Fixture {
    std::stringstream strm;
    OutputArchive output_archive{strm};
};

}  // namespace

SCENARIO_METHOD(Fixture, "Point serialization") {
    GIVEN("A point") {
        const Point p{10, 20};
        WHEN("point is serialized") {
            output_archive << p;

            THEN("it is equal to point after serialization") {
                InputArchive input_archive{strm};
                Point restored_point;
                input_archive >> restored_point;
                CHECK(p == restored_point);
            }
        }
    }
}

SCENARIO_METHOD(Fixture, "Dog Serialization") {
    GIVEN("a dog") {
        const auto dog = [] {
            Dog dog{Dog::Id{42}, "Pluto"s, {42.2, 12.5}, 3};
            dog.AddScore(42);
            dog.CollectItem(LostObject{LostObject::Id{10}, 2, {1.0, 2.0}, 5});
            dog.SetDirection(Dog::Direction::EAST);
            dog.SetDogSpeed({2.3, -1.2});
            return dog;
        }();

        WHEN("dog is serialized") {
            {
                serialization::DogRepr repr{dog};
                output_archive << repr;
            }

            THEN("it can be deserialized") {
                InputArchive input_archive{strm};
                serialization::DogRepr repr;
                input_archive >> repr;
                const auto restored = repr.Restore();

                CHECK(dog.GetId() == restored->GetId());
                CHECK(dog.GetName() == restored->GetName());
                CHECK(dog.GetDogPosition().x == restored->GetDogPosition().x);
                CHECK(dog.GetDogPosition().y == restored->GetDogPosition().y);
                CHECK(dog.GetDogSpeed().v_x == restored->GetDogSpeed().v_x);
                CHECK(dog.GetDogSpeed().v_y == restored->GetDogSpeed().v_y);
                CHECK(dog.GetBag().GetCapacity() == restored->GetBag().GetCapacity());
                
                // Проверка содержимого сумки
                const auto& original_items = dog.GetBag().GetItems();
                const auto& restored_items = restored->GetBag().GetItems();
                CHECK(original_items.size() == restored_items.size());
                if (!original_items.empty()) {
                    CHECK(original_items[0].GetId() == restored_items[0].GetId());
                    CHECK(original_items[0].GetType() == restored_items[0].GetType());
                    CHECK(original_items[0].GetPosition().x == restored_items[0].GetPosition().x);
                    CHECK(original_items[0].GetPosition().y == restored_items[0].GetPosition().y);
                    CHECK(original_items[0].GetValue() == restored_items[0].GetValue());
                }
            }
        }
    }
}
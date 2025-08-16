#define _USE_MATH_DEFINES
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <catch2/matchers/catch_matchers_range_equals.hpp>
#include <catch2/matchers/catch_matchers_templated.hpp>

#include "../src/collision_detector.h"

#include <string>
#include <memory>
#include <algorithm>
#include <sstream>

// Напишите здесь тесты для функции collision_detector::FindGatherEvents

namespace {
    const std::string TAG = "[FindGatherEvents]";
}

namespace collision_detector {

class ItemGathererProviderImpl: public ItemGathererProvider {
public:
    virtual ~ItemGathererProviderImpl() = default;
    
    size_t ItemsCount() const {
        return items_.size();
    };

    Item GetItem(size_t idx) const override{
        return items_.at(idx);
    };

    void AddItem(Item item) {
        items_.push_back(std::move(item));
    };

    size_t GatherersCount() const override{
        return gatherers_.size();
    };

    Gatherer GetGatherer(size_t idx) const override{
        return gatherers_.at(idx);
    };

    void AddGatherer(Gatherer gatherer) {
        gatherers_.push_back(std::move(gatherer));
    };
private:
    std::vector<Item> items_;
    std::vector<Gatherer> gatherers_;
};

}  // namespace collision_detector

namespace Catch {
template<>
struct StringMaker<collision_detector::GatheringEvent> {
  static std::string convert(collision_detector::GatheringEvent const& value) {
      std::ostringstream tmp;
      tmp << "(" << value.gatherer_id << "," << value.item_id << "," << value.sq_distance << "," << value.time << ")";
      return tmp.str();
  }
};
}  // namespace Catch

namespace {

template <typename Range, typename Predicate>
struct EqualsRangeMatcher : Catch::Matchers::MatcherGenericBase {
    EqualsRangeMatcher(Range const& range, Predicate predicate)
        : range_{range}
        , predicate_{predicate} {
    }

    template <typename OtherRange>
    bool match(const OtherRange& other) const {
        using std::begin;
        using std::end;

        return std::equal(begin(range_), end(range_), begin(other), end(other), predicate_);
    }

    std::string describe() const override {
        return "Equals: " + Catch::rangeToString(range_);
    }

private:
    const Range& range_;
    Predicate predicate_;
};

template <typename Range, typename Predicate>
auto EqualsRange(const Range& range, Predicate prediate) {
    return EqualsRangeMatcher<Range, Predicate>{range, prediate};
}

class VectorItemGathererProvider : public collision_detector::ItemGathererProvider {
public:
    VectorItemGathererProvider(std::vector<collision_detector::Item> items,
                               std::vector<collision_detector::Gatherer> gatherers)
        : items_(items)
        , gatherers_(gatherers) {
    }

    
    size_t ItemsCount() const override {
        return items_.size();
    }
    collision_detector::Item GetItem(size_t idx) const override {
        return items_[idx];
    }
    size_t GatherersCount() const override {
        return gatherers_.size();
    }
    collision_detector::Gatherer GetGatherer(size_t idx) const override {
        return gatherers_[idx];
    }

private:
    std::vector<collision_detector::Item> items_;
    std::vector<collision_detector::Gatherer> gatherers_;
};

class CompareEvents {
public:
    bool operator()(const collision_detector::GatheringEvent& l,
                    const collision_detector::GatheringEvent& r) {
        if (l.gatherer_id != r.gatherer_id || l.item_id != r.item_id) 
            return false;

        static const double eps = 1e-10;

        if (std::abs(l.sq_distance - r.sq_distance) > eps) {
            return false;
        }

        if (std::abs(l.time - r.time) > eps) {
            return false;
        }
        return true;
    }
};

}

using namespace std::literals;

TEST_CASE("No collisions when no movement", TAG) {
    collision_detector::Item item{{12.5, 0}, 0.6};
    collision_detector::Gatherer gatherer{{0, 0}, {0, 0}, 0.6};  // No movement
    collision_detector::ItemGathererProviderImpl provider;
    provider.AddItem(item);
    provider.AddGatherer(gatherer);
    auto events = collision_detector::FindGatherEvents(provider);
    
    CHECK(events.empty());
}

TEST_CASE("No collisions when items are too far", TAG) {
    collision_detector::Item item{{100, 100}, 0.6};
    collision_detector::Gatherer gatherer{{0, 0}, {10, 0}, 0.6};
    collision_detector::ItemGathererProviderImpl provider;
    provider.AddItem(item);
    provider.AddGatherer(gatherer);
    auto events = collision_detector::FindGatherEvents(provider);
    
    CHECK(events.empty());
}

TEST_CASE("Gather collect one item moving diagonally", TAG) {
    using Catch::Matchers::WithinRel;
    collision_detector::Item item{{5, 5}, 0.6};
    collision_detector::Gatherer gatherer{{0, 0}, {10, 10}, 0.6};
    collision_detector::ItemGathererProviderImpl provider;
    provider.AddItem(item);
    provider.AddGatherer(gatherer);
    auto events = collision_detector::FindGatherEvents(provider);
    
    CHECK(events.size() == 1);
    CHECK(events[0].item_id == 0);
    CHECK(events[0].gatherer_id == 0);
    CHECK_THAT(events[0].sq_distance, WithinRel(0.0, 1e-10));
    CHECK_THAT(events[0].time, WithinRel(0.5, 1e-10));
}

TEST_CASE("Multiple gatherers and items complex scenario", TAG) {
    using Catch::Matchers::WithinRel;
    collision_detector::ItemGathererProviderImpl provider;
    
    // Add items
    provider.AddItem({{2, 0}, 0.5});    // item 0
    provider.AddItem({{4, 1}, 0.5});     // item 1
    provider.AddItem({{6, 0}, 0.5});     // item 2
    provider.AddItem({{8, -1}, 0.5});    // item 3
    
    // Add gatherers
    provider.AddGatherer({{0, 0}, {10, 0}, 0.6});  // gatherer 0 - along x-axis
    provider.AddGatherer({{5, -5}, {5, 5}, 0.6});   // gatherer 1 - along y-axis
    
    auto events = collision_detector::FindGatherEvents(provider);
    
    REQUIRE(events.size() == 6);
    
    // Check chronological order
    for (size_t i = 1; i < events.size(); ++i) {
        CHECK(events[i].time >= events[i-1].time);
    }
    
    // Check specific events
    CHECK(std::count_if(events.begin(), events.end(), [](const auto& e) {
        return e.gatherer_id == 0 && e.item_id == 0;
    }) == 1);
    
    CHECK(std::count_if(events.begin(), events.end(), [](const auto& e) {
        return e.gatherer_id == 0 && e.item_id == 1;
    }) == 1);
    
    CHECK(std::count_if(events.begin(), events.end(), [](const auto& e) {
        return e.gatherer_id == 0 && e.item_id == 2;
    }) == 1);
    
    CHECK(std::count_if(events.begin(), events.end(), [](const auto& e) {
        return e.gatherer_id == 0 && e.item_id == 3;
    }) == 1);

    CHECK(std::count_if(events.begin(), events.end(), [](const auto& e) {
        return e.gatherer_id == 1 && e.item_id == 1;
    }) == 1);
    
    CHECK(std::count_if(events.begin(), events.end(), [](const auto& e) {
        return e.gatherer_id == 1 && e.item_id == 2;
    }) == 1);
}

TEST_CASE("Edge case - touching exactly at boundary", TAG) {
    using Catch::Matchers::WithinRel;
    collision_detector::Item item{{10, 0}, 0.5};
    collision_detector::Gatherer gatherer{{0, 0}, {10, 0}, 0.5};
    collision_detector::ItemGathererProviderImpl provider;
    provider.AddItem(item);
    provider.AddGatherer(gatherer);
    auto events = collision_detector::FindGatherEvents(provider);
    
    CHECK(events.size() == 1);
    CHECK_THAT(events[0].time, WithinRel(1.0, 1e-10));
}

TEST_CASE("Different sized items and gatherers", TAG) {
    using Catch::Matchers::WithinRel;
    collision_detector::Item item1{{5, 0}, 1.0};    // Large item
    collision_detector::Item item2{{10, 5}, 0.1};   // Small item
    collision_detector::Gatherer gatherer1{{0, 0}, {20, 0}, 0.5};  // Medium gatherer
    collision_detector::Gatherer gatherer2{{10, 0}, {10, 10}, 1.0}; // Large gatherer
    
    collision_detector::ItemGathererProviderImpl provider;
    provider.AddItem(item1);
    provider.AddItem(item2);
    provider.AddGatherer(gatherer1);
    provider.AddGatherer(gatherer2);
    
    auto events = collision_detector::FindGatherEvents(provider);
    
    REQUIRE(events.size() == 2);
    
    // Check all expected collisions are present
    CHECK(std::count_if(events.begin(), events.end(), [](const auto& e) {
        return e.gatherer_id == 0 && e.item_id == 0;
    }) == 1);
    
    CHECK(std::count_if(events.begin(), events.end(), [](const auto& e) {
        return e.gatherer_id == 1 && e.item_id == 1;
    }) == 1);
}

TEST_CASE("Gather collect one item moving on x-axis", TAG) {
    using Catch::Matchers::WithinRel;
    collision_detector::Item item{{12.5, 0}, 0.6};
    collision_detector::Gatherer gatherer{{0, 0}, {22.5, 0}, 0.6};
    collision_detector::ItemGathererProviderImpl provider;
    provider.AddItem(item);
    provider.AddGatherer(gatherer);
    auto events = collision_detector::FindGatherEvents(provider);
    
    CHECK(events.size() == 1);
    CHECK(events[0].item_id == 0);
    CHECK(events[0].gatherer_id == 0);
    CHECK_THAT(events[0].sq_distance, WithinRel(0.0, 1e-10));
    CHECK_THAT(events[0].time, WithinRel((item.position.x/gatherer.end_pos.x), 1e-10)); 
}

TEST_CASE("Gather collect one item moving on x-axis on edge", TAG) {
    using Catch::Matchers::WithinRel;
    collision_detector::Item item{{12.5, 0}, 0.6};
    collision_detector::Gatherer gatherer{{0, 0}, {12.5, 0}, 0.6};
    collision_detector::ItemGathererProviderImpl provider;
    provider.AddItem(item);
    provider.AddGatherer(gatherer);
    auto events = collision_detector::FindGatherEvents(provider);
    
    CHECK(events.size() == 1);
    CHECK(events[0].item_id == 0);
    CHECK(events[0].gatherer_id == 0);
    CHECK_THAT(events[0].sq_distance, WithinRel(0.0, 1e-10));
    CHECK_THAT(events[0].time, WithinRel((item.position.x/gatherer.end_pos.x), 1e-10)); 
}

TEST_CASE("Gather collect one item moving on x-axis on side", TAG) {
    using Catch::Matchers::WithinRel;
    collision_detector::Item item{{12.5, 0.5}, 0.0};
    collision_detector::Gatherer gatherer{{0, 0.1}, {22.5, 0.1}, 0.6};
    collision_detector::ItemGathererProviderImpl provider;
    provider.AddItem(item);
    provider.AddGatherer(gatherer);
    auto events = collision_detector::FindGatherEvents(provider);
    
    CHECK(events.size() == 1);
    CHECK(events[0].item_id == 0);
    CHECK(events[0].gatherer_id == 0);
    CHECK_THAT(events[0].sq_distance, WithinRel(0.16, 1e-10));
    CHECK_THAT(events[0].time, WithinRel((item.position.x/gatherer.end_pos.x), 1e-10)); 
}

TEST_CASE("Gather collect one item moving on y-axis", TAG) {
    using Catch::Matchers::WithinRel;
    collision_detector::Item item{{0, 12.5}, 0.6};
    collision_detector::Gatherer gatherer{{0, 0}, {0, 22.5}, 0.6};
    collision_detector::ItemGathererProviderImpl provider;
    provider.AddItem(item);
    provider.AddGatherer(gatherer);
    auto events = collision_detector::FindGatherEvents(provider);
    
    CHECK(events.size() == 1);
    CHECK(events[0].item_id == 0);
    CHECK(events[0].gatherer_id == 0);
    CHECK_THAT(events[0].sq_distance, WithinRel(0.0, 1e-10));
    CHECK_THAT(events[0].time, WithinRel((item.position.y/gatherer.end_pos.y), 1e-10)); 
}

TEST_CASE("Gather collect two unordered items moving on x-axis", TAG) {
    using Catch::Matchers::WithinRel;
    collision_detector::Item item1{{12.5, 0}, 0.6};
    collision_detector::Item item2{{6.5, 0}, 0.6};
    collision_detector::Gatherer gatherer{{0, 0}, {22.5, 0}, 0.6};
    collision_detector::ItemGathererProviderImpl provider;
    provider.AddItem(item1);
    provider.AddItem(item2);
    provider.AddGatherer(gatherer);
    auto events = collision_detector::FindGatherEvents(provider);
    
    CHECK(events.size() == 2);

    CHECK(events[0].item_id == 1);
    CHECK(events[0].gatherer_id == 0);
    CHECK_THAT(events[0].sq_distance, WithinRel(0.0, 1e-10));
    CHECK_THAT(events[0].time, WithinRel((item2.position.x/gatherer.end_pos.x), 1e-10)); 
    
    CHECK(events[1].item_id == 0);
    CHECK(events[1].gatherer_id == 0);
    CHECK_THAT(events[1].sq_distance, WithinRel(0.0, 1e-10));
    CHECK_THAT(events[1].time, WithinRel((item1.position.x/gatherer.end_pos.x), 1e-10)); 
}

TEST_CASE("Gather collect one of two items moving on x-axis", TAG) {
    using Catch::Matchers::WithinRel;
    collision_detector::Item item1{{42.5, 0}, 0.6};
    collision_detector::Item item2{{6.5, 0}, 0.6};
    collision_detector::Gatherer gatherer{{0, 0}, {22.5, 0}, 0.6};
    collision_detector::ItemGathererProviderImpl provider;
    provider.AddItem(item1);
    provider.AddItem(item2);
    provider.AddGatherer(gatherer);
    auto events = collision_detector::FindGatherEvents(provider);

    CHECK(events.size() == 1);

    CHECK(events[0].item_id == 1);
    CHECK(events[0].gatherer_id == 0);
    CHECK_THAT(events[0].sq_distance, WithinRel(0.0, 1e-10));
    CHECK_THAT(events[0].time, WithinRel((item2.position.x/gatherer.end_pos.x), 1e-10)); 
}

TEST_CASE("Two gathers collect two separate items moving on x-axis and y-axis", TAG) {
    using Catch::Matchers::WithinRel;
    collision_detector::Item item1{{0, 12.5}, 0.6};
    collision_detector::Item item2{{6.5, 0}, 0.6};
    collision_detector::Gatherer gatherer1{{0, 0}, {22.5, 0}, 0.6};
    collision_detector::Gatherer gatherer2{{0, 0}, {0, 22.5}, 0.6};
    collision_detector::ItemGathererProviderImpl provider;
    provider.AddItem(item1);
    provider.AddItem(item2);
    provider.AddGatherer(gatherer1);
    provider.AddGatherer(gatherer2);
    auto events = collision_detector::FindGatherEvents(provider);
    
    CHECK(events.size() == 2);

    CHECK(events[0].item_id == 1);
    CHECK(events[0].gatherer_id == 0);
    CHECK_THAT(events[0].sq_distance, WithinRel(0.0, 1e-10));
    CHECK_THAT(events[0].time, WithinRel((item2.position.x/gatherer1.end_pos.x), 1e-10)); 
    
    CHECK(events[1].item_id == 0);
    CHECK(events[1].gatherer_id == 1);
    CHECK_THAT(events[1].sq_distance, WithinRel(0.0, 1e-10));
    CHECK_THAT(events[1].time, WithinRel((item1.position.y/gatherer2.end_pos.y), 1e-10)); 
}

TEST_CASE("Two gathers collect three items moving on x-axis and y-axis", TAG) {
    using Catch::Matchers::WithinRel;
    collision_detector::Item item1{{12.5, 0}, 0.6};
    collision_detector::Item item2{{6.5, 0}, 0.6};
    collision_detector::Gatherer gatherer1{{0, 0}, {22.5, 0}, 0.6};
    collision_detector::Gatherer gatherer2{{0, 0}, {10, 0}, 0.6};
    collision_detector::ItemGathererProviderImpl provider;
    provider.AddItem(item1);
    provider.AddItem(item2);
    provider.AddGatherer(gatherer1);
    provider.AddGatherer(gatherer2);
    auto events = collision_detector::FindGatherEvents(provider);
    
    CHECK(events.size() == 3);

    CHECK(events[0].item_id == 1);
    CHECK(events[0].gatherer_id == 0);
    CHECK_THAT(events[0].sq_distance, WithinRel(0.0, 1e-10));
    CHECK_THAT(events[0].time, WithinRel((item2.position.x/gatherer1.end_pos.x), 1e-10)); 

    CHECK(events[1].item_id == 0);
    CHECK(events[1].gatherer_id == 0);
    CHECK_THAT(events[1].sq_distance, WithinRel(0.0, 1e-10));
    CHECK_THAT(events[1].time, WithinRel((item1.position.x/gatherer1.end_pos.x), 1e-10)); 

    CHECK(events[2].item_id == 1);
    CHECK(events[2].gatherer_id == 1);
    CHECK_THAT(events[2].sq_distance, WithinRel(0.0, 1e-10));
    CHECK_THAT(events[2].time, WithinRel((item2.position.x/gatherer2.end_pos.x), 1e-10)); 
}

SCENARIO("Collision detection") {
    WHEN("no items") {
        VectorItemGathererProvider provider{
            {}, {{{1, 2}, {4, 2}, 5.}, {{0, 0}, {10, 10}, 5.}, {{-5, 0}, {10, 5}, 5.}}};
        THEN("No events") {
            auto events = collision_detector::FindGatherEvents(provider);
            CHECK(events.empty());
        }
    }
    WHEN("no gatherers") {
        VectorItemGathererProvider provider{
            {{{1, 2}, 5.}, {{0, 0}, 5.}, {{-5, 0}, 5.}}, {}};
        THEN("No events") {
            auto events = collision_detector::FindGatherEvents(provider);
            CHECK(events.empty());
        }
    }
    WHEN("multiple items on a way of gatherer") {
        VectorItemGathererProvider provider{{
            {{9, 0.27}, .1},
            {{8, 0.24}, .1},
            {{7, 0.21}, .1},
            {{6, 0.18}, .1},
            {{5, 0.15}, .1},
            {{4, 0.12}, .1},
            {{3, 0.09}, .1},
            {{2, 0.06}, .1},
            {{1, 0.03}, .1},
            {{0, 0.0}, .1},
            {{-1, 0}, .1},
            }, {
            {{0, 0}, {10, 0}, 0.1},
        }};
        THEN("Gathered items in right order") {
            auto events = collision_detector::FindGatherEvents(provider);
            CHECK_THAT(
                events,
                EqualsRange(std::vector{
                    collision_detector::GatheringEvent{9, 0,0.*0., 0.0},
                    collision_detector::GatheringEvent{8, 0,0.03*0.03, 0.1},
                    collision_detector::GatheringEvent{7, 0,0.06*0.06, 0.2},
                    collision_detector::GatheringEvent{6, 0,0.09*0.09, 0.3},
                    collision_detector::GatheringEvent{5, 0,0.12*0.12, 0.4},
                    collision_detector::GatheringEvent{4, 0,0.15*0.15, 0.5},
                    collision_detector::GatheringEvent{3, 0,0.18*0.18, 0.6},
                }, CompareEvents()));
        }
    }
    WHEN("multiple gatherers and one item") {
        VectorItemGathererProvider provider{{
                                                {{0, 0}, 0.},
                                            },
                                            {
                                                {{-5, 0}, {5, 0}, 1.},
                                                {{0, 1}, {0, -1}, 1.},
                                                {{-10, 10}, {101, -100}, 0.5}, // <-- that one
                                                {{-100, 100}, {10, -10}, 0.5},
                                            }
        };
        THEN("Item gathered by faster gatherer") {
            auto events = collision_detector::FindGatherEvents(provider);
            CHECK(events.front().gatherer_id == 2);
        }
    }
    WHEN("Gatherers stay put") {
        VectorItemGathererProvider provider{{
                                                {{0, 0}, 10.},
                                            },
                                            {
                                                {{-5, 0}, {-5, 0}, 1.},
                                                {{0, 0}, {0, 0}, 1.},
                                                {{-10, 10}, {-10, 10}, 100}
                                            }
        };
        THEN("No events detected") {
            auto events = collision_detector::FindGatherEvents(provider);

            CHECK(events.empty());
        }
    }
}
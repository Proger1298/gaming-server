#include "collision_detector.h"

#include <algorithm>
#include <cassert>
#include <vector>

namespace collision_detector {

CollectionResult TryCollectPoint(geom::Point2D a, geom::Point2D b, geom::Point2D c) {
    // Проверим, что перемещение ненулевое.
    // Тут приходится использовать строгое равенство, а не приближённое,
    // пскольку при сборе заказов придётся учитывать перемещение даже на небольшое
    // расстояние.
    assert(b.x != a.x || b.y != a.y);
    const double u_x = c.x - a.x;
    const double u_y = c.y - a.y;
    const double v_x = b.x - a.x;
    const double v_y = b.y - a.y;
    const double u_dot_v = u_x * v_x + u_y * v_y;
    const double u_len2 = u_x * u_x + u_y * u_y;
    const double v_len2 = v_x * v_x + v_y * v_y;
    const double proj_ratio = u_dot_v / v_len2;
    const double sq_distance = u_len2 - (u_dot_v * u_dot_v) / v_len2;

    return CollectionResult(sq_distance, proj_ratio);
}

std::vector<GatheringEvent> FindGatherEvents(const ItemGathererProvider& provider) {
    std::vector<GatheringEvent> events;

    for (size_t gatherer_id = 0; gatherer_id < provider.GatherersCount(); ++gatherer_id) {
        const auto gatherer = provider.GetGatherer(gatherer_id);
        
        // Пропускаем неподвижных собирателей
        if (gatherer.start_pos == gatherer.end_pos) {
            continue;
        }

        for (size_t item_id = 0; item_id < provider.ItemsCount(); ++item_id) {
            const auto item = provider.GetItem(item_id);
            
            // Проверяем возможность сбора точки
            auto collect_result = TryCollectPoint(
                gatherer.start_pos, 
                gatherer.end_pos, 
                item.position
            );

            // Проверяем попадание в область сбора
            double total_radius = gatherer.width + item.width;
            if (collect_result.IsCollected(total_radius)) {
                events.push_back({
                    item_id,
                    gatherer_id,
                    collect_result.sq_distance,
                    collect_result.proj_ratio
                });
            }
        }
    }

    // Сортируем события по времени (proj_ratio)
    std::sort(events.begin(), events.end(), [](const GatheringEvent& lhs, const GatheringEvent& rhs) {
        return lhs.time < rhs.time;
    });

    return events;
}

}  // namespace collision_detector
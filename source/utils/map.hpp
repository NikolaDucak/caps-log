#pragma once

namespace caps_log::utils {

template <class MapT, typename ConditionT> inline void mapRemoveIf(MapT &map, ConditionT cond) {
    for (auto it = map.begin(); it != map.end();) {
        if (cond(*it)) {
            it = map.erase(it);
        } else {
            it++;
        }
    }
}

} // namespace caps_log::utils

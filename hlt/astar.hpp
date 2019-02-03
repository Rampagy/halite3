#include "types.hpp"
#include "direction.hpp"
#include "position.hpp"
#include "game_map.hpp"

#include <unordered_map>
#include <unordered_set>
#include <cmath>
#include <queue>
#include <memory>

using namespace std;
using namespace hlt;

typedef pair<float, Position> score_T;


Direction astar(const unordered_map<int, unordered_map<int, Halite>> halite_map,
                const Position start, const Position goal,
                const unordered_set<Position>& occupied_spaces,
                const unique_ptr<GameMap>& game_map,
                const unordered_map<EntityId, Position>& dropoffs,
                const unordered_map<EntityId, Position>& other_dropoffs,
                const unordered_map<EntityId, Position>& undesireable,
                const unordered_map<EntityId, shared_ptr<Ship>>& other_ships,
                Halite& path_halite, int& distance, bool end_game=false,
                bool target_is_drop=false);

float heuristic(const Position a, const Position b);

#include "astar.hpp"

float heuristic(Position a, Position b, const unique_ptr<GameMap>& game_map)
{
    return (float)game_map->calculate_distance(a, b);
}

Direction astar(unordered_map<int, unordered_map<int, Halite>> halite_map,
                const Position start, const Position goal,
                const unordered_set<Position>& occupied_spaces,
                const unique_ptr<GameMap>& game_map,
                const unordered_map<EntityId, Position>& dropoffs,
                const unordered_map<EntityId, Position>& other_dropoffs,
                const unordered_map<EntityId, Position>& undesireable,
                const unordered_map<EntityId, shared_ptr<Ship>>& other_ships,
                Halite& path_halite, int& distance, bool end_game, bool target_is_drop)
{
    if (start != goal)
    {
        unordered_set<Position> close_set;
        unordered_map<Position, Position> came_from;
        unordered_map<Position, float> gscore;
        unordered_map<Position, float> fscore;
        priority_queue<score_T, vector<score_T>, greater<score_T>> oheap;
        unordered_set<Position> oheap_copy;
        unordered_set<Position> unwalkable;

        bool goal_is_drop = false;
        for (const auto& drop_iter : dropoffs)
        {
            if (drop_iter.second == goal)
            {
                goal_is_drop = true;
                break;
            }
        }

        for (const Position occ_pos : occupied_spaces)
        {
            if (!end_game || !game_map->at(occ_pos)->has_structure())
            {
                bool found = false;
                if (goal_is_drop)
                {
                    for (const auto& drop_iter : dropoffs)
                    {
                        if (drop_iter.second == occ_pos)
                        {
                            found = true;
                            break;
                        }
                    }
                }

                // do not add if occ_pos is drop off and goal to start distance is > 2
                if ((!found) || (game_map->calculate_distance(start, goal) < 2))
                {
                    unwalkable.insert(occ_pos);
                }
            }
        }

        for (const auto& drop_iter : dropoffs)
        {
            halite_map[drop_iter.second.y][drop_iter.second.x] += (Halite)1000;
        }

        for (const auto&  drop_iter : other_dropoffs)
        {
            halite_map[drop_iter.second.y][drop_iter.second.x] += (Halite)1000;
        }

        for (const auto& undesired : undesireable)
        {
            halite_map[undesired.second.y][undesired.second.x] += (Halite)1000;
        }

        for (const auto& o_ship_iter : other_ships)
        {
            unwalkable.insert(o_ship_iter.second->position);
            for (Position p : o_ship_iter.second->position.get_surrounding_cardinals())
            {
                p = game_map->normalize(p);
                if (target_is_drop && (game_map->calculate_distance(start, goal) > 6))
                {
                    unwalkable.insert(p);
                }
                else
                {
                    halite_map[p.y][p.x] += (Halite)500;
                }
            }
        }

        gscore[start] = 0;
        fscore[start] = heuristic(start, goal, game_map);
        oheap.push(make_pair(fscore[start], start));
        oheap_copy.insert(start);

        while(!oheap.empty())
        {
            Position current = oheap.top().second;
            oheap.pop();
            oheap_copy.erase(current);

            if (current == goal)
            {
                vector<Position> data;
                while (came_from.find(current) != came_from.end())
                {
                    data.push_back(current);
                    current = came_from[current];
                    path_halite += game_map->at(current)->halite;
                    distance++;
                }
                // can only move 1 spot per turn, so only ret that spot
                Direction dir = Direction::STILL;
                for (const auto& curr_dir : ALL_CARDINALS)
                {
                    Position check_pos = game_map->normalize(
                                            start.directional_offset(curr_dir));
                    if (check_pos == data.back())
                    {
                        dir =  curr_dir;
                    }
                }
                return dir;
            }

            close_set.insert(current);
            array<Position, 4> neighbors = current.get_surrounding_cardinals();
            for (Position neighbor : neighbors)
            {
                neighbor = game_map->normalize(neighbor);
                if (unwalkable.find(neighbor) != unwalkable.end())
                {
                    continue;
                }

                float tentative_g_score = gscore[current] + 300 +
                                        halite_map[current.y][current.x];
                float current_gscore;
                if (gscore.find(neighbor) == gscore.end())
                {
                    current_gscore = 0;
                }
                else
                {
                    current_gscore = gscore.find(neighbor)->second;
                }

                if ((close_set.find(neighbor) != close_set.end()) &&
                    (tentative_g_score >= current_gscore))
                {
                    continue;
                }

                if ((tentative_g_score < current_gscore) ||
                    (oheap_copy.find(neighbor) == oheap_copy.end()))
                {
                    came_from[neighbor] = current;
                    gscore[neighbor] = tentative_g_score;
                    fscore[neighbor] = tentative_g_score + heuristic(neighbor, goal, game_map);
                    oheap.push(make_pair(fscore[neighbor], neighbor));
                    if (oheap_copy.find(neighbor) == oheap_copy.end())
                    {
                        // don't add twice (keys must be unique)
                        oheap_copy.insert(neighbor);
                    }
                }
            }
        }
    }
    return Direction::STILL;
}

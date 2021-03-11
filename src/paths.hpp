#ifndef RMP_PATHS_HPP
#define RMP_PATHS_HPP

#include <string>

#include "puzzles.hpp"

namespace paths {

#ifdef RESIM
const std::string PUZZLE_DATA = ".";
#else
const std::string PUZZLE_DATA = "/opt/etc/puzzles";
#endif

inline std::string game_basename(const game *g)
{
    // this seems to be the field with the most "standard" version of the
    // game's name (i.e. the same name as the c file)
    return g->htmlhelp_topic;
}

inline std::string game_config(const game *g)
{
    return PUZZLE_DATA + "/config/" + game_basename(g) + ".ini";
}

inline std::string game_help(const game *g)
{
    return PUZZLE_DATA + "/help/" + game_basename(g) + ".txt";
}

inline std::string game_save(const game *g)
{
    return PUZZLE_DATA + "/save/" + game_basename(g) + ".sav";
}

inline std::string icon(const std::string & name)
{
    return PUZZLE_DATA + "/icons/" + name + ".png";
}

inline std::string game_icon(const game *g)
{
    return icon(game_basename(g));
}

} // namespace

#endif // RMP_PATHS_HPP

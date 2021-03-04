#ifndef RMP_GAME_LIST_HPP
#define RMP_GAME_LIST_HPP

#include "puzzles.hpp"

#define GAMELIST(X) \
    X(blackbox) \
    X(blackbox) \
    X(bridges) \
    X(cube) \
    X(galaxies) \
    X(inertia) \
    X(lightup) \
    X(mines) \
    X(net) \
    X(pearl) \
    X(pegs) \
    X(samegame) \
    X(slant) \
    X(tents) \
    X(tracks) \
    X(unruly) \
    X(untangle) \


#define GAME_DECL(x) extern const game x;
#define GAME_REF(x) &x,
GAMELIST(GAME_DECL)
static const game *GAME_LIST[] = { GAMELIST(GAME_REF) };

#undef GAMELIST
#undef GAME_DECL
#undef GAME_REF

#endif // RMP_GAME_LIST_HPP

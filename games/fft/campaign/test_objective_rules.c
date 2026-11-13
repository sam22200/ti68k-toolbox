// Independent policy boundaries; no renderer, archive migration or runtime integration.
#include <assert.h>
#include <stdio.h>
#define FFT_BATTLE
#include "../fft.h"
#include "../objective_rules.h"
int main(void)
{
    assert(core_objective_valid(1,1U<<12,13)); // slot0 is a valid target
    assert(core_objective_valid(0,0,13));
    assert(!core_objective_valid(3,0,13));
    assert(!core_objective_valid(1,1,13));
    assert(!core_objective_valid(1U<<13,0,13));
    assert(!core_objective_valid(0,0,0));
    assert(!core_objective_valid(0,0,14));
    assert(core_objective_result(0,0,0,0,0,END_NONE)==END_NONE);
    assert(core_objective_result(0,0,0,0,0,END_VICTORY)==END_VICTORY);
    assert(core_objective_result(0,0,0,0,0,END_DEFEAT)==END_DEFEAT);
    assert(core_objective_result(1,0,1,0,0,END_NONE)==END_VICTORY);
    assert(core_objective_result(1,0,0,1,0,END_NONE)==END_VICTORY);
    assert(core_objective_result(1,0,0,0,0,END_VICTORY)==END_NONE); // living named target
    assert(core_objective_result(1,0,0,0,0,END_DEFEAT)==END_DEFEAT);
    assert(core_objective_result(1,2,3,0,0,END_VICTORY)==END_DEFEAT);
    assert(core_objective_result(1,2,1,0,1,END_VICTORY)==END_DEFEAT);
    assert(core_objective_result(0,1U<<12,1U<<12,0,0,END_VICTORY)==END_DEFEAT);
    assert(core_objective_result(0,2,0,2,0,END_NONE)==END_DEFEAT);
    puts("objective policy:18 boundaries pass (prepared; not integrated)");
    return 0;
}

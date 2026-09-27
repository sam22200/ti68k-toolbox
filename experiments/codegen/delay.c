// Old games time themselves with empty loops. At -Os GCC4TI removes them entirely (f compiles to rts).
void f(short speed) { short i, j; for (i = 0; i < speed * 10; i++) for (j = 0; j < 1000; j++) {} }

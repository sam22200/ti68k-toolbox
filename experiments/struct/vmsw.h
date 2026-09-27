// Switch-dispatched interpreter body, included twice by vm.c: once plain (VM_COUNT empty), once
// counting the executed opcodes. Runs one entity until its next YIELD (or HALT).
static __attribute__((noinline)) void VM_NAME(VEnt *e, const unsigned char *base)
{
    const unsigned char *pc = base + e->pc;
    char *v = (char *)e->v;
    for (;;) {
        VM_COUNT;
        switch (*pc++) {
        case OP_LDI:  R(pc[0]) = (signed char)pc[1]; pc += 2; break;
        case OP_MOV:  R(pc[0]) = R(pc[1]); pc += 2; break;
        case OP_ADD:  R(pc[0]) += R(pc[1]); pc += 2; break;
        case OP_SUB:  R(pc[0]) -= R(pc[1]); pc += 2; break;
        case OP_ADDI: R(pc[0]) += (signed char)pc[1]; pc += 2; break;
        case OP_ANDI: R(pc[0]) &= (signed char)pc[1]; pc += 2; break;
        case OP_LDB:  R(pc[0]) = (signed char)base[pc[2] + R(pc[1])]; pc += 3; break;
        case OP_JMP:  pc += 1 + (signed char)pc[0]; break;
        case OP_JZ:   pc += 2; if (!R(pc[-2])) pc += (signed char)pc[-1]; break;
        case OP_JNZ:  pc += 2; if (R(pc[-2])) pc += (signed char)pc[-1]; break;
        case OP_DJNZ: pc += 2; if (--R(pc[-2])) pc += (signed char)pc[-1]; break;
        case OP_JLTI: pc += 3; if (R(pc[-3]) < (signed char)pc[-2]) pc += (signed char)pc[-1]; break;
        case OP_JEQ:  pc += 3; if (R(pc[-3]) == R(pc[-2])) pc += (signed char)pc[-1]; break;
        case OP_JLT:  pc += 3; if (R(pc[-3]) < R(pc[-2])) pc += (signed char)pc[-1]; break;
        case OP_YIELD: e->pc = pc - base; return;
        case OP_CALL: natives[*pc++](e->v); break;
        default:      e->pc = pc - 1 - base; return;         // HALT: stay on it
        }
    }
}

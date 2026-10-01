#include "../h/riscv.hpp"
#include "../h/kprint.hpp"

int main() {
    kprintString("Test ispisa\n");
    kprintUInt(0);        kprintString("\n");   // 0
    kprintUInt(255);      kprintString("\n");   // 255
    kprintUInt(255, 16);  kprintString("\n");   // 0xFF
    kprintInt(-7);        kprintString("\n");   // -7
    Riscv::haltEmulator();
    return 0;
}
#include <fstream>
#include "2513ROM.hpp"

int main(  ) {
    auto bank = {
        _AT, _UA, _UB, _UC, _UD, _UE, _UF, _UG,
        _UH, _UI, _UJ, _UK, _UL, _UM, _UN, _UO,
        _UP, _UQ, _UR, _US, _UT, _UU, _UV, _UW,
        _UX, _UY, _UZ, _OB, _BL, _CB, _PW, _NS,
        _SP, _EX, _DQ, _HS, _DO, _PC, _ND, _QT,
        _OP, _CO, _AK, _PL, _CM, _MN, _DT, _SL,
        _N0, _N1, _N2, _N3, _N4, _N5, _N6, _N7,
        _N8, _N9, _DD, _DC, _LT, _EQ, _GT, _QS
    };

    char assoc[] = {
        '@', 'A', 'B', 'C', 'D', 'E', 'F', 'G',
        'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
        'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W',
        'X', 'Y', 'Z', '[', '\\', ']', '^', '_',
        ' ', '!', '"', '#', '$', '%', '&', '\'',
        '(', ')', '*', '+', ',', '-', '.', '/',
        '0', '1', '2', '3', '4', '5', '6', '7',
        '8', '9', ':', ';', '<', '=', '>', '?'
    };

    fstream file;


    size_t i = 0;
    for (auto it=bank.begin(); it != bank.end(); ++it) {
        const char fname[1024] = { 0 };
        sprintf(const_cast<char *>(fname), "./2513/%02ldtest.svg", i);
        printf("%s\n", fname );
        file.open(fname, ios::out );
        printf("// %c\n", assoc[i]);
        printPath(file, *it);
        ++i;
        file.close();
    }
    return 0;
}
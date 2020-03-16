//
// Created by sebastian on 28/2/20.
//

#ifndef SDL_CRT_FILTER_2513ROM_HPP
#define SDL_CRT_FILTER_2513ROM_HPP

#define CHAR_W 5
#define CHAR_H 8

#include <map>
#include <iostream>
using namespace std;


typedef char charLine[CHAR_W + 1];
typedef const char* character[CHAR_H];

const charLine blank = ".....";

const character _AT = {
        blank,
        ".zzz.",
        "z...z",
        "z.z.z",
        "z.zzz",
        "z.zz.",
        "z....",
        ".zzzz"
};

const character _UA = {
        blank,
        "..z..",
        ".z.z.",
        "z...z",
        "z...z",
        "zzzzz",
        "z...z",
        "z...z"
};

const character _UB = {
        blank,
        "zzzz.",
        "z...z",
        "z...z",
        "zzzz.",
        "z...z",
        "z...z",
        "zzzz."
};

const character _UC = {
        blank,
        ".zzz.",
        "z...z",
        "z....",
        "z....",
        "z....",
        "z...z",
        ".zzz."
};

const character _UD = {
        blank,
        "zzzz.",
        "z...z",
        "z...z",
        "z...z",
        "z...z",
        "z...z",
        "zzzz."
};

const character _UE = {
        blank,
        "zzzzz",
        "z....",
        "z....",
        "zzzz.",
        "z....",
        "z....",
        "zzzzz"
};

const character _UF = {
        blank,
        "zzzzz",
        "z....",
        "z....",
        "zzzz.",
        "z....",
        "z....",
        "z...."
};

const character _UG = {
        blank,
        ".zzzz",
        "z....",
        "z....",
        "z....",
        "z..zz",
        "z...z",
        ".zzzz"
};

//################
const character _UH = {
        blank,
        "z...z",
        "z...z",
        "z...z",
        "zzzzz",
        "z...z",
        "z...z",
        "z...z"
};

const character _UI = {
        blank,
        ".zzz.",
        "..z..",
        "..z..",
        "..z..",
        "..z..",
        "..z..",
        ".zzz."
};

const character _UJ = {
        blank,
        "....z",
        "....z",
        "....z",
        "....z",
        "....z",
        "z...z",
        ".zzz."
};

const character _UK = {
        blank,
        "k...k",
        "k..k.",
        "k.k..",
        "kk...",
        "k.k..",
        "k..k.",
        "k...k"
};

const character _UL = {
        blank,
        "k....",
        "k....",
        "k....",
        "k....",
        "k....",
        "k....",
        "kkkkk"
};

const character _UM = {
        blank,
        "m...m",
        "mm.mm",
        "m.m.m",
        "m.m.m",
        "m...m",
        "m...m",
        "m...m"
};

const character _UN = {
        blank,
        "n...n",
        "n...n",
        "nn..n",
        "n.n.n",
        "n..nn",
        "n...n",
        "n...n"
};

const character _UO = {
        blank,
        ".nnn.",
        "n...n",
        "n...n",
        "n...n",
        "n...n",
        "n...n",
        ".nnn."
};

//################
const character _UP = {
        blank,
        "nnnn.",
        "n...n",
        "n...n",
        "nnnn.",
        "n....",
        "n....",
        "n...."
};

const character _UQ = {
        blank,
        ".nnn.",
        "n...n",
        "n...n",
        "n...n",
        "n.n.n",
        "n..n.",
        ".nn.n"
};

const character _UR = {
        blank,
        "rrrr.",
        "r...r",
        "r...r",
        "rrrr.",
        "r.r..",
        "r..r.",
        "r...r"
};

const character _US = {
        blank,
        ".rrr.",
        "r...r",
        "r....",
        ".rrr.",
        "....r",
        "r...r",
        ".rrr."
};

const character _UT = {
        blank,
        "rrrrr",
        "..r..",
        "..r..",
        "..r..",
        "..r..",
        "..r..",
        "..r.."
};

const character _UU = {
        blank,
        "u...u",
        "u...u",
        "u...u",
        "u...u",
        "u...u",
        "u...u",
        ".uuu."
};

const character _UV = {
        blank,
        "u...u",
        "u...u",
        "u...u",
        "u...u",
        "u...u",
        ".u.u.",
        "..u.."
};

const character _UW = {
        blank,
        "x...x",
        "x...x",
        "x...x",
        "x.x.x",
        "x.x.x",
        "xx.xx",
        "x...x"
};


//################
const character _UX = {
        blank,
        "x...x",
        "x...x",
        ".x.x.",
        "..x..",
        ".x.x.",
        "x...x",
        "x...x"
};

const character _UY = {
        blank,
        "x...x",
        "x...x",
        ".x.x.",
        "..x..",
        "..x..",
        "..x..",
        "..x.."
};

const character _UZ = {
        blank,
        "xxxxx",
        "....z",
        "...z.",
        "..z..",
        ".z...",
        "z....",
        "xxxxx"
};

const character _OB = {
        blank,
        "zzzzz",
        "zz...",
        "zz...",
        "zz...",
        "zz...",
        "zz...",
        "zzzzz"
};

const character _BL = {
        blank,
        ".....",
        "z....",
        ".z...",
        "..z..",
        "...z.",
        "....z",
        "....."
};

const character _CB = {
        blank,
        "zzzzz",
        "...zz",
        "...zz",
        "...zz",
        "...zz",
        "...zz",
        "zzzzz"
};

const character _PW = {
        blank,
        ".....",
        ".....",
        "..z..",
        ".z.z.",
        "z...z",
        ".....",
        "....."
};

const character _NS = {
        blank,
        ".....",
        ".....",
        ".....",
        ".....",
        ".....",
        ".....",
        "zzzzz"
};

//################
const character _SP = {
        blank,
        ".....",
        ".....",
        ".....",
        ".....",
        ".....",
        ".....",
        "....."
};

const character _EX = {
        blank,
        "..z..",
        "..z..",
        "..z..",
        "..z..",
        "..z..",
        ".....",
        "..z.."
};

const character _DQ = {
        blank,
        ".z.z.",
        ".z.z.",
        ".z.z.",
        ".....",
        ".....",
        ".....",
        "....."
};

const character _HS = {
        blank,
        ".z.z.",
        ".z.z.",
        "zzzzz",
        ".z.z.",
        "zzzzz",
        ".z.z.",
        ".z.z."
};

const character _DO = {
        blank,
        "..z..",
        ".zzzz",
        "z.z..",
        ".zzz.",
        "..z.z",
        "zzzz.",
        "..z.."
};

const character _PC = {
        blank,
        "zz...",
        "zz..z",
        "...z.",
        "..z..",
        ".z...",
        "z..zz",
        "...zz"
};

const character _ND = {
        blank,
        ".z...",
        "z.z..",
        "z.z..",
        ".z...",
        "z.z.z",
        "z..z.",
        ".zz.z"
};

const character _QT = {
        blank,
        "..z..",
        "..z..",
        "..z..",
        ".....",
        ".....",
        ".....",
        "....."
};

//################
const character _OP = {
        blank,
        "..c..",
        ".c...",
        "c....",
        "c....",
        "c....",
        ".c...",
        "..c.."
};

const character _CO = {
        blank,
        "..c..",
        "...c.",
        "....c",
        "....c",
        "....c",
        "...c.",
        "..c.."
};

const character _AK = {
        blank,
        "..c..",
        "c.c.c",
        ".ccc.",
        "..c..",
        ".ccc.",
        "c.c.c",
        "..c.."
};

const character _PL = {
        blank,
        ".....",
        "..c..",
        "..c..",
        "ccccc",
        "..c..",
        "..c..",
        "....."
};

const character _CM = {
        blank,
        ".....",
        ".....",
        ".....",
        ".....",
        "..c..",
        "..c..",
        ".c..."
};

const character _MN = {
        blank,
        ".....",
        ".....",
        ".....",
        "ccccc",
        ".....",
        ".....",
        "....."
};

const character _DT = {
        blank,
        ".....",
        ".....",
        ".....",
        ".....",
        ".....",
        ".....",
        "..c.."
};

const character _SL = {
        blank,
        ".....",
        "....c",
        "...c.",
        "..c..",
        ".c...",
        "c....",
        "....."
};

//################
const character _N0 = {
        blank,
        ".ccc.",
        "c...c",
        "c..cc",
        "c.c.c",
        "cc..c",
        "c...c",
        ".ccc."
};

const character _N1 = {
        blank,
        "..c..",
        ".cc..",
        "..c..",
        "..c..",
        "..c..",
        "..c..",
        ".ccc."
};

const character _N2 = {
        blank,
        ".ccc.",
        "c...c",
        "....c",
        "..cc.",
        ".c...",
        "c....",
        "ccccc"
};

const character _N3 = {
        blank,
        "ccccc",
        "....c",
        "...c.",
        "..cc.",
        "....c",
        "c...c",
        ".ccc."
};

const character _N4 = {
        blank,
        "...m.",
        "..mm.",
        ".m.m.",
        "m..m.",
        "mmmmm",
        "...m.",
        "...m."
};

const character _N5 = {
        blank,
        "mmmmm",
        "m....",
        "mmmm.",
        "....m",
        "....m",
        "m...m",
        ".mmm."
};

const character _N6 = {
        blank,
        "..kkk",
        ".k...",
        "k....",
        "kkkk.",
        "k...k",
        "k...k",
        ".kkk."
};

const character _N7 = {
        blank,
        "77777",
        "....7",
        "...7.",
        "..7..",
        ".7...",
        ".7...",
        ".7..."
};

//################
const character _N8 = {
        blank,
        ".zzz.",
        "z...z",
        "z...z",
        ".zzz.",
        "z...z",
        "z...z",
        ".zzz."
};

const character _N9 = {
        blank,
        ".zzz.",
        "z...z",
        "z...z",
        ".zzzz",
        "....z",
        "...z.",
        "zzz.."
};

const character _DD = {
        blank,
        ".....",
        ".....",
        "..x..",
        ".....",
        "..x..",
        ".....",
        "....."
};

const character _DC = {
        blank,
        ".....",
        ".....",
        "..x..",
        ".....",
        "..x..",
        "..x..",
        ".x..."
};

const character _LT = {
        blank,
        "...x.",
        "..x..",
        ".x...",
        "x....",
        ".x...",
        "..x..",
        "...x."
};

const character _EQ = {
        blank,
        ".....",
        ".....",
        "xxxxx",
        ".....",
        "xxxxx",
        ".....",
        "....."
};

const character _GT = {
        blank,
        ".x...",
        "..x..",
        "...x.",
        "....x",
        "...x.",
        "..x..",
        ".x..."
};

const character _QS = {
        blank,
        ".xxx.",
        "x...x",
        "...x.",
        "..x..",
        "..x..",
        ".....",
        "..x.."
};




void printChar(const character c) {
    for (int i = 0; i < CHAR_H; ++i) {
        for (int j = 0; j < CHAR_W; ++j)
            if (c[i][j] == '.')
                printf("  ");
            else
                printf("##");
        printf("\n");
    }
}

void SVGheader(fstream& file) {
    const std::string head = "<?xml version=\"1.0\" standalone=\"no\"?>\n" \
                  "<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\" >\n" \
                  "\t<svg xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" version=\"1.1\" viewBox=\"0 -205 768 1024\">" \
                  "\t\t<g transform=\"matrix(1 0 0 -1 0 819)\">" \
                  "\t\t\t<path fill=\"currentColor\" " \
                  "d=\"";
    file << head.c_str();
    //printf("%s", head.c_str());
}

void SVGfooter(fstream& file) {
    std::string footer = "\" />\n"\
        "\t\t</g>"\
        "</svg>";
    file << footer.c_str();
    //printf("%s", footer.c_str());
}

void printPath(fstream& file, const character c) {
    size_t dotSize = 128;
    SVGheader(file);
    for (int i = 0; i < CHAR_H; ++i) {
        for (int j = 0; j < CHAR_W; ++j)
            if (c[i][j] != '.') {
                const char format[10240] = { 0 };
                size_t sx = j * dotSize;
                size_t sy = ((CHAR_H - i) * dotSize) - dotSize;
                sprintf(const_cast<char *>(format), "M%zu %zu ", sx, sy); file << format;
                sprintf(const_cast<char *>(format), "h%zu", dotSize); file << format;
                sprintf(const_cast<char *>(format), "v%zu", dotSize); file << format;
                sprintf(const_cast<char *>(format), "h-%zu", dotSize); file << format;
                sprintf(const_cast<char *>(format), "v-%zu", dotSize); file << format;
                sprintf(const_cast<char *>(format), "Z "); file << format;
            }
    }
    SVGfooter(file);
    printf("\n");
}

#endif //SDL_CRT_FILTER_2513ROM_HPP

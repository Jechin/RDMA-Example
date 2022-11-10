/*
 * @Description: Implement in c++
 * @Autor: zltang
 * @Date: 2022-11-10 15:44:21
 * @LastEditors: Jechin
 * @LastEditTime: 2022-11-10 19:00:25
 */

#include <iostream>
#include <fstream>

#define SS1(A, E, j) ("(((" + A + "<<<12)+" + E + "+(T_" + to_string(j) + "<<<" + to_string(j) + "))<<<7)")
#define SS2(A, SS1Str) ("(" + SS1Str + "^" + "(" + A + "<<<12))")
#define TT1(A, B, C, D, SS2Str, W_, j) ("(" + (j <= 15 ? A + "^" + B + "^" + C : "(" + A + "&" + B + ")" + "|" + "(" + A + "&" + C + ")" + "|" + "(" + B + "&" + C + ")") + "+" + D + "+" + SS2Str + "+" + W_ + to_string(j) + ")")
#define TT2(E, F, G, H, SS1Str, W, j) ("(" + (j <= 15 ? E + "^" + F + "^" + G : "(" + E + "&" + F + ")" + "|" + "(~" + E + "&" + G + ")") + "+" + H + "+" + SS1Str + "+" + W + to_string(j) + ")")
#define D(C) "(" + C +  ")"
#define C(B) (B + "<<<9")
#define B(A) "(" + A  + ")"
#define A(TT1Str) TT1Str
#define H(G) "(" + G + ")"
#define G(F) ("(" + F + "<<<19)")
#define F(E) "(" + E + ")"
#define E(TT2Str) ("((" + TT2Str + ")^((" + TT2Str + ")<<<9)^((" + TT2Str + ")<<<17))")

using namespace std;

int main()
{
    string A = "A_0", B = "B_0", C = "C_0", D = "D_0", E = "E_0", F = "F_0", G = "G_0", H = "H_0", T = "T_0", W = "W_", W_ = "W'_";
    ofstream fileIn;
    fileIn.open("output.txt");
    if(!fileIn.is_open()) return 1;
    for (int j = 0; j < 8; j++)
    {
        string SS1 = SS1(A, E, j);
        string SS2 = SS2(A, SS1);
        string TT1 = TT1(A, B, C, D, SS2, W_, j);
        string TT2 = TT2(E, F, G, H, SS1, W, j);
        D = D(C);
        C = C(B);
        B = B(A);
        A = A(TT1);
        H = H(G);
        G = G(F);
        F = F(E);
        E = E(TT2);
        fileIn << "*************************j = " << j <<  "*****************" << endl;
        fileIn << "A = " << A << endl;
        fileIn << "B = " << B << endl;
        fileIn << "C = " << C << endl;
        fileIn << "D = " << D << endl;
        fileIn << "E = " << E << endl;
        fileIn << "F = " << F << endl;
        fileIn << "G = " << G << endl;
        fileIn << "H = " << H << endl;
        // fileIn << "SS1 = " << SS1 << endl;
        // fileIn << "SS2 = " << SS2 << endl;
        // fileIn << "TT1 = " << TT1 << endl;
        // fileIn << "TT1 = " << TT2 << endl;
        fileIn << endl;
        

    }
    fileIn.close();
    return 0;
}
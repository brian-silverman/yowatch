#include "_fonts.h"

static const uint16 space[8][5] = {
    { _ _ _ _ _ },
    { _ _ _ _ _ },
    { _ _ _ _ _ },
    { _ _ _ _ _ },
    { _ _ _ _ _ },
    { _ _ _ _ _ },
    { _ _ _ _ _ },
    { _ _ _ _ _ },
};
static const uint16 n_space[8][3] = {
    { _ _ _ },
    { _ _ _ },
    { _ _ _ },
    { _ _ _ },
    { _ _ _ },
    { _ _ _ },
    { _ _ _ },
    { _ _ _ },
};
static const uint16 excl[8][5] = {
    { _ _ M _ _ },
    { _ _ M _ _ },
    { _ _ M _ _ },
    { _ _ M _ _ },
    { _ _ M _ _ },
    { _ _ _ _ _ },
    { _ _ M _ _ },
    { _ _ _ _ _ },
};
static const uint16 n_excl[8][1] = {
    { M },
    { M },
    { M },
    { M },
    { M },
    { _ },
    { M },
    { _ },
};
static const uint16 dquote[8][5] = {
    { _ M _ M _ },
    { _ M _ M _ },
    { _ M _ M _ },
    { _ _ _ _ _ },
    { _ _ _ _ _ },
    { _ _ _ _ _ },
    { _ _ _ _ _ },
    { _ _ _ _ _ },
};
static const uint16 n_dquote[8][3] = {
    { M _ M },
    { M _ M },
    { M _ M },
    { _ _ _ },
    { _ _ _ },
    { _ _ _ },
    { _ _ _ },
    { _ _ _ },
};
static const uint16 pound[8][5] = {
    { _ M _ M _ },
    { _ M _ M _ },
    { M M M M M },
    { _ M _ M _ },
    { M M M M M },
    { _ M _ M _ },
    { _ M _ M _ },
    { _ _ _ _ _ },
};
static const uint16 dollar[8][5] = {
    { _ _ M _ _ },
    { _ M M M M },
    { M _ M _ _ },
    { _ M M M _ },
    { _ _ M _ M },
    { M M M M _ },
    { _ _ M _ _ },
    { _ _ _ _ _ },
};
static const uint16 percent[8][5] = {
    { M M _ _ _ },
    { M M _ _ M },
    { _ _ _ M _ },
    { _ _ M _ _ },
    { _ M _ _ _ },
    { M _ _ M M },
    { _ _ _ M M },
    { _ _ _ _ _ },
};
static const uint16 and[8][5] = {
    { _ M _ _ _ },
    { M _ M _ _ },
    { M _ M _ _ },
    { _ M _ _ _ },
    { M _ M _ M },
    { M _ _ M _ },
    { _ M M _ M },
    { _ _ _ _ _ },
};
static const uint16 squote[8][5] = {
    { _ _ M M _ },
    { _ _ M M _ },
    { _ _ M _ _ },
    { _ M _ _ _ },
    { _ _ _ _ _ },
    { _ _ _ _ _ },
    { _ _ _ _ _ },
    { _ _ _ _ _ },
};
static const uint16 n_squote[8][3] = {
    { _ M M },
    { _ M M },
    { _ M _ },
    { M _ _ },
    { _ _ _ },
    { _ _ _ },
    { _ _ _ },
    { _ _ _ },
};
static const uint16 lparen[8][5] = {
    { _ _ _ M _ },
    { _ _ M _ _ },
    { _ M _ _ _ },
    { _ M _ _ _ },
    { _ M _ _ _ },
    { _ _ M _ _ },
    { _ _ _ M _ },
    { _ _ _ _ _ },
};
static const uint16 n_lparen[8][3] = {
    { _ _ M },
    { _ M _ },
    { M _ _ },
    { M _ _ },
    { M _ _ },
    { _ M _ },
    { _ _ M },
    { _ _ _ },
};
static const uint16 rparen[8][5] = {
    { _ M _ _ _ },
    { _ _ M _ _ },
    { _ _ _ M _ },
    { _ _ _ M _ },
    { _ _ _ M _ },
    { _ _ M _ _ },
    { _ M _ _ _ },
    { _ _ _ _ _ },
};
static const uint16 n_rparen[8][3] = {
    { M _ _ },
    { _ M _ },
    { _ _ M },
    { _ _ M },
    { _ _ M },
    { _ M _ },
    { M _ _ },
    { _ _ _ },
};
static const uint16 star[8][5] = {
    { _ _ M _ _ },
    { M _ M _ M },
    { _ M M M _ },
    { M M M M M },
    { _ M M M _ },
    { M _ M _ M },
    { _ _ M _ _ },
    { _ _ _ _ _ },
};
static const uint16 plus[8][5] = {
    { _ _ _ _ _ },
    { _ _ M _ _ },
    { _ _ M _ _ },
    { M M M M M },
    { _ _ M _ _ },
    { _ _ M _ _ },
    { _ _ _ _ _ },
    { _ _ _ _ _ },
};
static const uint16 comma[8][5] = {
    { _ _ _ _ _ },
    { _ _ _ _ _ },
    { _ _ _ _ _ },
    { _ _ _ _ _ },
    { _ _ M M _ },
    { _ _ M M _ },
    { _ _ M _ _ },
    { _ M _ _ _ },
};
static const uint16 n_comma[8][3] = {
    { _ _ _ },
    { _ _ _ },
    { _ _ _ },
    { _ _ _ },
    { _ M M },
    { _ M M },
    { _ M _ },
    { M _ _ },
};
static const uint16 dash[8][5] = {
    { _ _ _ _ _ },
    { _ _ _ _ _ },
    { _ _ _ _ _ },
    { M M M M M },
    { _ _ _ _ _ },
    { _ _ _ _ _ },
    { _ _ _ _ _ },
    { _ _ _ _ _ },
};
static const uint16 dot[8][5] = {
    { _ _ _ _ _ },
    { _ _ _ _ _ },
    { _ _ _ _ _ },
    { _ _ _ _ _ },
    { _ _ _ _ _ },
    { _ _ M M _ },
    { _ _ M M _ },
    { _ _ _ _ _ },
};
static const uint16 n_dot[8][2] = {
    { _ _ },
    { _ _ },
    { _ _ },
    { _ _ },
    { _ _ },
    { M M },
    { M M },
    { _ _ },
};
static const uint16 fslash[8][5] = {
    { _ _ _ _ _ },
    { _ _ _ _ M },
    { _ _ _ M _ },
    { _ _ M _ _ },
    { _ M _ _ _ },
    { M _ _ _ _ },
    { _ _ _ _ _ },
    { _ _ _ _ _ },
};
static const uint16 n0[8][5] = {
    { _ M M M _ },
    { M _ _ _ M },
    { M _ _ M M },
    { M _ M _ M },
    { M M _ _ M },
    { M _ _ _ M },
    { _ M M M _ },
    { _ _ _ _ _ },
};
static const uint16 n1[8][5] = {
    { _ _ M _ _ },
    { _ M M _ _ },
    { _ _ M _ _ },
    { _ _ M _ _ },
    { _ _ M _ _ },
    { _ _ M _ _ },
    { _ M M M _ },
    { _ _ _ _ _ },
};
static const uint16 n_n1[8][3] = {
    { _ M _ },
    { M M _ },
    { _ M _ },
    { _ M _ },
    { _ M _ },
    { _ M _ },
    { M M M },
    { _ _ _ },
};
static const uint16 n2[8][5] = {
    { _ M M M _ },
    { M _ _ _ M },
    { _ _ _ _ M },
    { _ M M M _ },
    { M _ _ _ _ },
    { M _ _ _ _ },
    { M M M M M },
    { _ _ _ _ _ },
};
static const uint16 n3[8][5] = {
    { M M M M M },
    { _ _ _ _ M },
    { _ _ _ M _ },
    { _ _ M M _ },
    { _ _ _ _ M },
    { M _ _ _ M },
    { _ M M M _ },
    { _ _ _ _ _ },
};
static const uint16 n4[8][5] = {
    { _ _ _ M _ },
    { _ _ M M _ },
    { _ M _ M _ },
    { M _ _ M _ },
    { M M M M M },
    { _ _ _ M _ },
    { _ _ _ M _ },
    { _ _ _ _ _ },
};
static const uint16 n5[8][5] = {
    { M M M M M },
    { M _ _ _ _ },
    { M M M M _ },
    { _ _ _ _ M },
    { _ _ _ _ M },
    { M _ _ _ M },
    { _ M M M _ },
    { _ _ _ _ _ },
};
static const uint16 n6[8][5] = {
    { _ _ M M M },
    { _ M _ _ _ },
    { M _ _ _ _ },
    { M M M M _ },
    { M _ _ _ M },
    { M _ _ _ M },
    { _ M M M _ },
    { _ _ _ _ _ },
};
static const uint16 n7[8][5] = {
    { M M M M M },
    { _ _ _ _ M },
    { _ _ _ _ M },
    { _ _ _ M _ },
    { _ _ M _ _ },
    { _ M _ _ _ },
    { M _ _ _ _ },
    { _ _ _ _ _ },
};
static const uint16 n8[8][5] = {
    { _ M M M _ },
    { M _ _ _ M },
    { M _ _ _ M },
    { _ M M M _ },
    { M _ _ _ M },
    { M _ _ _ M },
    { _ M M M _ },
    { _ _ _ _ _ },
};
static const uint16 n9[8][5] = {
    { _ M M M _ },
    { M _ _ _ M },
    { M _ _ _ M },
    { _ M M M M },
    { _ _ _ _ M },
    { _ _ _ M _ },
    { M M M _ _ },
    { _ _ _ _ _ },
};
static const uint16 colon[8][5] = {
    { _ _ _ _ _ },
    { _ _ _ _ _ },
    { _ _ M _ _ },
    { _ _ _ _ _ },
    { _ _ M _ _ },
    { _ _ _ _ _ },
    { _ _ _ _ _ },
    { _ _ _ _ _ },
};
static const uint16 n_colon[8][1] = {
    { _ },
    { _ },
    { M },
    { _ },
    { M },
    { _ },
    { _ },
    { _ },
};
static const uint16 scolon[8][5] = {
    { _ _ _ _ _ },
    { _ _ _ _ _ },
    { _ _ M _ _ },
    { _ _ _ _ _ },
    { _ _ M _ _ },
    { _ _ M _ _ },
    { _ M _ _ _ },
    { _ _ _ _ _ },
};
static const uint16 n_scolon[8][2] = {
    { _ _ },
    { _ _ },
    { _ M },
    { _ _ },
    { _ M },
    { _ M },
    { M _ },
    { _ _ },
};
static const uint16 le[8][5] = {
    { _ _ _ _ M },
    { _ _ _ M _ },
    { _ _ M _ _ },
    { _ M _ _ _ },
    { _ _ M _ _ },
    { _ _ _ M _ },
    { _ _ _ _ M },
    { _ _ _ _ _ },
};
static const uint16 n_le[8][4] = {
    { _ _ _ M },
    { _ _ M _ },
    { _ M _ _ },
    { M _ _ _ },
    { _ M _ _ },
    { _ _ M _ },
    { _ _ _ M },
    { _ _ _ _ },
};
static const uint16 eq[8][5] = {
    { _ _ _ _ _ },
    { _ _ _ _ _ },
    { M M M M M },
    { _ _ _ _ _ },
    { M M M M M },
    { _ _ _ _ _ },
    { _ _ _ _ _ },
    { _ _ _ _ _ },
};
static const uint16 gt[8][5] = {
    { _ M _ _ _ },
    { _ _ M _ _ },
    { _ _ _ M _ },
    { _ _ _ _ M },
    { _ _ _ M _ },
    { _ _ M _ _ },
    { _ M _ _ _ },
    { _ _ _ _ _ },
};
static const uint16 n_gt[8][4] = {
    { M _ _ _ },
    { _ M _ _ },
    { _ _ M _ },
    { _ _ _ M },
    { _ _ M _ },
    { _ M _ _ },
    { M _ _ _ },
    { _ _ _ _ },
};
static const uint16 question[8][5] = {
    { _ M M M _ },
    { M _ _ _ M },
    { _ _ _ _ M },
    { _ _ M M _ },
    { _ _ M _ _ },
    { _ _ _ _ _ },
    { _ _ M _ _ },
    { _ _ _ _ _ },
};
static const uint16 at[8][5] = {
    { _ M M M _ },
    { M _ _ _ M },
    { M _ M _ M },
    { M _ M M M },
    { M _ M M _ },
    { M _ _ _ _ },
    { _ M M M M },
    { _ _ _ _ _ },
};
static const uint16 A[8][5] = {
    { _ _ M _ _ },
    { _ M _ M _ },
    { M _ _ _ M },
    { M _ _ _ M },
    { M M M M M },
    { M _ _ _ M },
    { M _ _ _ M },
    { _ _ _ _ _ },
};
static const uint16 B[8][5] = {
    { M M M M _ },
    { M _ _ _ M },
    { M _ _ _ M },
    { M M M M _ },
    { M _ _ _ M },
    { M _ _ _ M },
    { M M M M _ },
    { _ _ _ _ _ },
};
static const uint16 C[8][5] = {
    { _ M M M _ },
    { M _ _ _ M },
    { M _ _ _ _ },
    { M _ _ _ _ },
    { M _ _ _ _ },
    { M _ _ _ M },
    { _ M M M _ },
    { _ _ _ _ _ },
};
static const uint16 D[8][5] = {
    { M M M M _ },
    { M _ _ _ M },
    { M _ _ _ M },
    { M _ _ _ M },
    { M _ _ _ M },
    { M _ _ _ M },
    { M M M M _ },
    { _ _ _ _ _ },
};
static const uint16 E[8][5] = {
    { M M M M M },
    { M _ _ _ _ },
    { M _ _ _ _ },
    { M M M M _ },
    { M _ _ _ _ },
    { M _ _ _ _ },
    { M M M M M },
    { _ _ _ _ _ },
};
static const uint16 F[8][5] = {
    { M M M M M },
    { M _ _ _ _ },
    { M _ _ _ _ },
    { M M M M _ },
    { M _ _ _ _ },
    { M _ _ _ _ },
    { M _ _ _ _ },
    { _ _ _ _ _ },
};
static const uint16 G[8][5] = {
    { _ M M M M },
    { M _ _ _ M },
    { M _ _ _ _ },
    { M _ _ _ _ },
    { M _ _ M M },
    { M _ _ _ M },
    { _ M M M M },
    { _ _ _ _ _ },
};
static const uint16 H[8][5] = {
    { M _ _ _ M },
    { M _ _ _ M },
    { M _ _ _ M },
    { M M M M M },
    { M _ _ _ M },
    { M _ _ _ M },
    { M _ _ _ M },
    { _ _ _ _ _ },
};
static const uint16 I[8][5] = {
    { _ M M M _ },
    { _ _ M _ _ },
    { _ _ M _ _ },
    { _ _ M _ _ },
    { _ _ M _ _ },
    { _ _ M _ _ },
    { _ M M M _ },
    { _ _ _ _ _ },
};
static const uint16 n_I[8][3] = {
    { M M M },
    { _ M _ },
    { _ M _ },
    { _ M _ },
    { _ M _ },
    { _ M _ },
    { M M M },
    { _ _ _ },
};
static const uint16 J[8][5] = {
    { _ _ M M M },
    { _ _ _ M _ },
    { _ _ _ M _ },
    { _ _ _ M _ },
    { _ _ _ M _ },
    { M _ _ M _ },
    { _ M M _ _ },
    { _ _ _ _ _ },
};
static const uint16 K[8][5] = {
    { M _ _ _ M },
    { M _ _ M _ },
    { M _ M _ _ },
    { M M _ _ _ },
    { M _ M _ _ },
    { M _ _ M _ },
    { M _ _ _ M },
    { _ _ _ _ _ },
};
static const uint16 L[8][5] = {
    { M _ _ _ _ },
    { M _ _ _ _ },
    { M _ _ _ _ },
    { M _ _ _ _ },
    { M _ _ _ _ },
    { M _ _ _ _ },
    { M M M M M },
    { _ _ _ _ _ },
};
// Poor letter M.  We use it as a macro, we define the variable M as _M
static const uint16 _M[8][5] = {
    { M _ _ _ M },
    { M M _ M M },
    { M _ M _ M },
    { M _ M _ M },
    { M _ M _ M },
    { M _ _ _ M },
    { M _ _ _ M },
    { _ _ _ _ _ },
};
static const uint16 N[8][5] = {
    { M _ _ _ M },
    { M _ _ _ M },
    { M M _ _ M },
    { M _ M _ M },
    { M _ _ M M },
    { M _ _ _ M },
    { M _ _ _ M },
    { _ _ _ _ _ },
};
static const uint16 O[8][5] = {
    { _ M M M _ },
    { M _ _ _ M },
    { M _ _ _ M },
    { M _ _ _ M },
    { M _ _ _ M },
    { M _ _ _ M },
    { _ M M M _ },
    { _ _ _ _ _ },
};
static const uint16 P[8][5] = {
    { M M M M _ },
    { M _ _ _ M },
    { M _ _ _ M },
    { M M M M _ },
    { M _ _ _ _ },
    { M _ _ _ _ },
    { M _ _ _ _ },
    { _ _ _ _ _ },
};
static const uint16 Q[8][5] = {
    { _ M M M _ },
    { M _ _ _ M },
    { M _ _ _ M },
    { M _ _ _ M },
    { M _ M _ M },
    { M _ _ M _ },
    { _ M M _ M },
    { _ _ _ _ _ },
};
static const uint16 R[8][5] = {
    { M M M M _ },
    { M _ _ _ M },
    { M _ _ _ M },
    { M M M M _ },
    { M _ M _ _ },
    { M _ _ M _ },
    { M _ _ _ M },
    { _ _ _ _ _ },
};
static const uint16 S[8][5] = {
    { _ M M M _ },
    { M _ _ _ M },
    { M _ _ _ _ },
    { _ M M M _ },
    { _ _ _ _ M },
    { M _ _ _ M },
    { _ M M M _ },
    { _ _ _ _ _ },
};
static const uint16 T[8][5] = {
    { M M M M M },
    { M _ M _ M },
    { _ _ M _ _ },
    { _ _ M _ _ },
    { _ _ M _ _ },
    { _ _ M _ _ },
    { _ _ M _ _ },
    { _ _ _ _ _ },
};
static const uint16 U[8][5] = {
    { M _ _ _ M },
    { M _ _ _ M },
    { M _ _ _ M },
    { M _ _ _ M },
    { M _ _ _ M },
    { M _ _ _ M },
    { _ M M M _ },
    { _ _ _ _ _ },
};
static const uint16 V[8][5] = {
    { M _ _ _ M },
    { M _ _ _ M },
    { M _ _ _ M },
    { M _ _ _ M },
    { M _ _ _ M },
    { _ M _ M _ },
    { _ _ M _ _ },
    { _ _ _ _ _ },
};
static const uint16 W[8][5] = {
    { M _ _ _ M },
    { M _ _ _ M },
    { M _ _ _ M },
    { M _ M _ M },
    { M _ M _ M },
    { M _ M _ M },
    { _ M _ M _ },
    { _ _ _ _ _ },
};
static const uint16 X[8][5] = {
    { M _ _ _ M },
    { M _ _ _ M },
    { _ M _ M _ },
    { _ _ M _ _ },
    { _ M _ M _ },
    { M _ _ _ M },
    { M _ _ _ M },
    { _ _ _ _ _ },
};
static const uint16 Y[8][5] = {
    { M _ _ _ M },
    { M _ _ _ M },
    { _ M _ M _ },
    { _ _ M _ _ },
    { _ _ M _ _ },
    { _ _ M _ _ },
    { _ _ M _ _ },
    { _ _ _ _ _ },
};
static const uint16 Z[8][5] = {
    { M M M M M },
    { _ _ _ _ M },
    { _ _ _ M _ },
    { _ M M M _ },
    { _ M _ _ _ },
    { M _ _ _ _ },
    { M M M M M },
    { _ _ _ _ _ },
};
static const uint16 lbrack[8][5] = {
    { _ M M M _ },
    { _ M _ _ _ },
    { _ M _ _ _ },
    { _ M _ _ _ },
    { _ M _ _ _ },
    { _ M _ _ _ },
    { _ M M M _ },
    { _ _ _ _ _ },
};
static const uint16 n_lbrack[8][3] = {
    { M M M },
    { M _ _ },
    { M _ _ },
    { M _ _ },
    { M _ _ },
    { M _ _ },
    { M M M },
    { _ _ _ },
};
static const uint16 bslash[8][5] = {
    { _ _ _ _ _ },
    { M _ _ _ _ },
    { _ M _ _ _ },
    { _ _ M _ _ },
    { _ _ _ M _ },
    { _ _ _ _ M },
    { _ _ _ _ _ },
    { _ _ _ _ _ },
};
static const uint16 rbrack[8][5] = {
    { _ M M M _ },
    { _ _ _ M _ },
    { _ _ _ M _ },
    { _ _ _ M _ },
    { _ _ _ M _ },
    { _ _ _ M _ },
    { _ M M M _ },
    { _ _ _ _ _ },
};
static const uint16 n_rbrack[8][3] = {
    { M M M },
    { _ _ M },
    { _ _ M },
    { _ _ M },
    { _ _ M },
    { _ _ M },
    { M M M },
    { _ _ _ },
};
static const uint16 hat[8][5] = {
    { _ _ M _ _ },
    { _ M _ M _ },
    { M _ _ _ M },
    { _ _ _ _ _ },
    { _ _ _ _ _ },
    { _ _ _ _ _ },
    { _ _ _ _ _ },
    { _ _ _ _ _ },
};
static const uint16 under[8][5] = {
    { _ _ _ _ _ },
    { _ _ _ _ _ },
    { _ _ _ _ _ },
    { _ _ _ _ _ },
    { _ _ _ _ _ },
    { _ _ _ _ _ },
    { M M M M M },
    { _ _ _ _ _ },
};
static const uint16 bquote[8][5] = {
    { _ M M _ _ },
    { _ M M _ _ },
    { _ _ M _ _ },
    { _ _ _ M _ },
    { _ _ _ _ _ },
    { _ _ _ _ _ },
    { _ _ _ _ _ },
    { _ _ _ _ _ },
};
static const uint16 n_bquote[8][3] = {
    { M M _ },
    { M M _ },
    { _ M _ },
    { _ _ M },
    { _ _ _ },
    { _ _ _ },
    { _ _ _ },
    { _ _ _ },
};
static const uint16 a[8][5] = {
    { _ _ _ _ _ },
    { _ _ _ _ _ },
    { _ M M _ _ },
    { _ _ _ M _ },
    { _ M M M _ },
    { M _ _ M _ },
    { _ M M M M },
    { _ _ _ _ _ },
};
static const uint16 b[8][5] = {
    { M _ _ _ _ },
    { M _ _ _ _ },
    { M _ M M _ },
    { M M _ _ M },
    { M _ _ _ M },
    { M M _ _ M },
    { M _ M M _ },
    { _ _ _ _ _ },
};
static const uint16 c[8][5] = {
    { _ _ _ _ _ },
    { _ _ _ _ _ },
    { _ M M M _ },
    { M _ _ _ M },
    { M _ _ _ _ },
    { M _ _ _ M },
    { _ M M M _ },
    { _ _ _ _ _ },
};
static const uint16 d[8][5] = {
    { _ _ _ _ M },
    { _ _ _ _ M },
    { _ M M _ M },
    { M _ _ M M },
    { M _ _ _ M },
    { M _ _ M M },
    { _ M M _ M },
    { _ _ _ _ _ },
};
static const uint16 e[8][5] = {
    { _ _ _ _ _ },
    { _ _ _ _ _ },
    { _ M M M _ },
    { M _ _ _ M },
    { M M M M M },
    { M _ _ _ _ },
    { _ M M M _ },
    { _ _ _ _ _ },
};
static const uint16 f[8][5] = {
    { _ _ _ M _ },
    { _ _ M _ M },
    { _ _ M _ _ },
    { _ M M M _ },
    { _ _ M _ _ },
    { _ _ M _ _ },
    { _ _ M _ _ },
    { _ _ _ _ _ },
};
static const uint16 n_f[8][4] = {
    { _ _ M _ },
    { _ M _ M },
    { _ M _ _ },
    { M M M _ },
    { _ M _ _ },
    { _ M _ _ },
    { _ M _ _ },
    { _ _ _ _ },
};
static const uint16 g[8][5] = {
    { _ _ _ _ _ },
    { _ _ _ _ _ },
    { _ M M M _ },
    { M _ _ M M },
    { M _ _ M M },
    { _ M M _ M },
    { _ _ _ _ M },
    { _ M M M _ },
};
static const uint16 h[8][5] = {
    { M _ _ _ _ },
    { M _ _ _ _ },
    { M _ M M _ },
    { M M _ _ M },
    { M _ _ _ M },
    { M _ _ _ M },
    { M _ _ _ M },
    { _ _ _ _ _ },
};
static const uint16 i[8][5] = {
    { _ _ M _ _ },
    { _ _ _ _ _ },
    { _ M M _ _ },
    { _ _ M _ _ },
    { _ _ M _ _ },
    { _ _ M _ _ },
    { _ M M M _ },
    { _ _ _ _ _ },
};
static const uint16 n_i[8][3] = {
    { _ M _ },
    { _ _ _ },
    { M M _ },
    { _ M _ },
    { _ M _ },
    { _ M _ },
    { M M M },
    { _ _ _ },
};
static const uint16 j[8][5] = {
    { _ _ _ _ _ },
    { _ _ _ M _ },
    { _ _ _ _ _ },
    { _ _ _ M _ },
    { _ _ _ M _ },
    { _ _ _ M _ },
    { M _ _ M _ },
    { _ M M _ _ },
};
static const uint16 n_j[8][4] = {
    { _ _ _ _ },
    { _ _ _ M },
    { _ _ _ _ },
    { _ _ _ M },
    { _ _ _ M },
    { _ _ _ M },
    { M _ _ M },
    { _ M M _ },
};
static const uint16 k[8][5] = {
    { M _ _ _ _ },
    { M _ _ _ _ },
    { M _ _ M _ },
    { M _ M _ _ },
    { M M _ _ _ },
    { M _ M _ _ },
    { M _ _ M _ },
    { _ _ _ _ _ },
};
static const uint16 n_k[8][4] = {
    { M _ _ _ },
    { M _ _ _ },
    { M _ _ M },
    { M _ M _ },
    { M M _ _ },
    { M _ M _ },
    { M _ _ M },
    { _ _ _ _ },
};
static const uint16 l[8][5] = {
    { _ M M _ _ },
    { _ _ M _ _ },
    { _ _ M _ _ },
    { _ _ M _ _ },
    { _ _ M _ _ },
    { _ _ M _ _ },
    { _ M M M _ },
    { _ _ _ _ _ },
};
static const uint16 n_l[8][3] = {
    { M M _ },
    { _ M _ },
    { _ M _ },
    { _ M _ },
    { _ M _ },
    { _ M _ },
    { M M M },
    { _ _ _ },
};
static const uint16 m[8][5] = {
    { _ _ _ _ _ },
    { _ _ _ _ _ },
    { M M _ M _ },
    { M _ M _ M },
    { M _ M _ M },
    { M _ M _ M },
    { M _ M _ M },
    { _ _ _ _ _ },
};
static const uint16 n[8][5] = {
    { _ _ _ _ _ },
    { _ _ _ _ _ },
    { M _ M M _ },
    { M M _ _ M },
    { M _ _ _ M },
    { M _ _ _ M },
    { M _ _ _ M },
    { _ _ _ _ _ },
};
static const uint16 o[8][5] = {
    { _ _ _ _ _ },
    { _ _ _ _ _ },
    { _ M M M _ },
    { M _ _ _ M },
    { M _ _ _ M },
    { M _ _ _ M },
    { _ M M M _ },
    { _ _ _ _ _ },
};
static const uint16 p[8][5] = {
    { _ _ _ _ _ },
    { _ _ _ _ _ },
    { M _ M M _ },
    { M M _ _ M },
    { M M _ _ M },
    { M _ M M _ },
    { M _ _ _ _ },
    { M _ _ _ _ },
};
static const uint16 q[8][5] = {
    { _ _ _ _ _ },
    { _ _ _ _ _ },
    { _ M M _ M },
    { M _ _ M M },
    { M _ _ M M },
    { _ M M _ M },
    { _ _ _ _ M },
    { _ _ _ _ M },
};
static const uint16 r[8][5] = {
    { _ _ _ _ _ },
    { _ _ _ _ _ },
    { M _ M M _ },
    { M M _ _ M },
    { M _ _ _ _ },
    { M _ _ _ _ },
    { M _ _ _ _ },
    { _ _ _ _ _ },
};
static const uint16 s[8][5] = {
    { _ _ _ _ _ },
    { _ _ _ _ _ },
    { _ M M M M },
    { M _ _ _ _ },
    { _ M M M _ },
    { _ _ _ _ M },
    { M M M M _ },
    { _ _ _ _ _ },
};
static const uint16 t[8][5] = {
    { _ _ M _ _ },
    { _ _ M _ _ },
    { M M M M M },
    { _ _ M _ _ },
    { _ _ M _ _ },
    { _ _ M _ M },
    { _ _ _ M _ },
    { _ _ _ _ _ },
};
static const uint16 u[8][5] = {
    { _ _ _ _ _ },
    { _ _ _ _ _ },
    { M _ _ _ M },
    { M _ _ _ M },
    { M _ _ _ M },
    { M _ _ M M },
    { _ M M _ M },
    { _ _ _ _ _ },
};
static const uint16 v[8][5] = {
    { _ _ _ _ _ },
    { _ _ _ _ _ },
    { M _ _ _ M },
    { M _ _ _ M },
    { M _ _ _ M },
    { _ M _ M _ },
    { _ _ M _ _ },
    { _ _ _ _ _ },
};
static const uint16 w[8][5] = {
    { _ _ _ _ _ },
    { _ _ _ _ _ },
    { M _ _ _ M },
    { M _ _ _ M },
    { M _ M _ M },
    { M _ M _ M },
    { _ M _ M _ },
    { _ _ _ _ _ },
};
static const uint16 x[8][5] = {
    { _ _ _ _ _ },
    { _ _ _ _ _ },
    { M _ _ _ M },
    { _ M _ M _ },
    { _ _ M _ _ },
    { _ M _ M _ },
    { M _ _ _ M },
    { _ _ _ _ _ },
};
static const uint16 y[8][5] = {
    { _ _ _ _ _ },
    { _ _ _ _ _ },
    { M _ _ _ M },
    { M _ _ _ M },
    { _ M M M M },
    { _ _ _ _ M },
    { M _ _ _ M },
    { _ M M M _ },
};
static const uint16 z[8][5] = {
    { _ _ _ _ _ },
    { _ _ _ _ _ },
    { M M M M M },
    { _ _ _ M _ },
    { _ _ M _ _ },
    { _ M _ _ _ },
    { M M M M M },
    { _ _ _ _ _ },
};
static const uint16 lsquig[8][5] = {
    { _ _ _ M _ },
    { _ _ M _ _ },
    { _ _ M _ _ },
    { _ M _ _ _ },
    { _ _ M _ _ },
    { _ _ M _ _ },
    { _ _ _ M _ },
    { _ _ _ _ _ },
};
static const uint16 n_lsquig[8][3] = {
    { _ _ M },
    { _ M _ },
    { _ M _ },
    { M _ _ },
    { _ M _ },
    { _ M _ },
    { _ _ M },
    { _ _ _ },
};
static const uint16 pipe[8][5] = {
    { _ _ M _ _ },
    { _ _ M _ _ },
    { _ _ M _ _ },
    { _ _ M _ _ },
    { _ _ M _ _ },
    { _ _ M _ _ },
    { _ _ M _ _ },
    { _ _ _ _ _ },
};
static const uint16 n_pipe[8][1] = {
    { M },
    { M },
    { M },
    { M },
    { M },
    { M },
    { M },
    { M },
};
static const uint16 rsquig[8][5] = {
    { _ M _ _ _ },
    { _ _ M _ _ },
    { _ _ M _ _ },
    { _ _ _ M _ },
    { _ _ M _ _ },
    { _ _ M _ _ },
    { _ M _ _ _ },
    { _ _ _ _ _ },
};
static const uint16 n_rsquig[8][3] = {
    { M _ _ },
    { _ M _ },
    { _ M _ },
    { _ _ M },
    { _ M _ },
    { _ M _ },
    { M _ _ },
    { _ _ _ },
};
static const uint16 tilde[8][5] = {
    { _ M _ _ _ },
    { M _ M _ M },
    { _ _ _ M _ },
    { _ _ _ _ _ },
    { _ _ _ _ _ },
    { _ _ _ _ _ },
    { _ _ _ _ _ },
    { _ _ _ _ _ },
};

static const uint16 box[8][5] = {
    { M M M M M },
    { M _ _ _ M },
    { M _ _ _ M },
    { M _ _ _ M },
    { M _ _ _ M },
    { M _ _ _ M },
    { M _ _ _ M },
    { M M M M M },
};

struct FONT_CHAR font_5x8_fixed[MAX_CHARS] = {
    // 0x00-0x0F
    F(box), F(box), F(box), F(box), F(box), F(box), F(box), F(box), 
    F(box), F(box), F(box), F(box), F(box), F(box), F(box), F(box), 

    // 0x10-0x1F
    F(box), F(box), F(box), F(box), F(box), F(box), F(box), F(box), 
    F(box), F(box), F(box), F(box), F(box), F(box), F(box), F(box), 

    // 0x20-0x2F
    F(space), F(excl), F(dquote), F(pound), F(dollar), F(percent), F(and), F(squote), 
    F(lparen), F(rparen), F(star), F(plus), F(comma), F(dash), F(dot), F(fslash),

    // 0x30-0x3F
    F(n0), F(n1), F(n2), F(n3), F(n4), F(n5), F(n6), F(n7),
    F(n8), F(n9), F(colon), F(scolon), F(le), F(eq), F(gt), F(question),

    // 0x40-0x4F
    F(at), F(A), F(B), F(C), F(D), F(E), F(F), F(G),
    F(H), F(I), F(J), F(K), F(L), F(_M), F(N), F(O),

    // 0x50-0x5F
    F(P), F(Q), F(R), F(S), F(T), F(U), F(V), F(W),
    F(X), F(Y), F(Z), F(lbrack), F(bslash), F(rbrack), F(hat), F(under),

    // 0x60-0x6F
    F(bquote), F(a), F(b), F(c), F(d), F(e), F(f), F(g),
    F(h), F(i), F(j), F(k), F(l), F(m), F(n), F(o),

    // 0x70-0x7F
    F(p), F(q), F(r), F(s), F(t), F(u), F(v), F(w),
    F(x), F(y), F(z), F(lsquig), F(pipe), F(rsquig), F(tilde), F(box),
};

struct FONT_CHAR font_5x8[MAX_CHARS] = {
    // 0x00-0x0F
    F(box), F(box), F(box), F(box), F(box), F(box), F(box), F(box), 
    F(box), F(box), F(box), F(box), F(box), F(box), F(box), F(box), 

    // 0x10-0x1F
    F(box), F(box), F(box), F(box), F(box), F(box), F(box), F(box), 
    F(box), F(box), F(box), F(box), F(box), F(box), F(box), F(box), 

    // 0x20-0x2F
    F(n_space), F(n_excl), F(n_dquote), F(pound), F(dollar), F(percent), F(and), F(n_squote), 
    F(n_lparen), F(n_rparen), F(star), F(plus), F(n_comma), F(dash), F(n_dot), F(fslash),

    // 0x30-0x3F
    F(n0), F(n_n1), F(n2), F(n3), F(n4), F(n5), F(n6), F(n7),
    F(n8), F(n9), F(n_colon), F(n_scolon), F(n_le), F(eq), F(n_gt), F(question),

    // 0x40-0x4F
    F(at), F(A), F(B), F(C), F(D), F(E), F(F), F(G),
    F(H), F(n_I), F(J), F(K), F(L), F(_M), F(N), F(O),

    // 0x50-0x5F
    F(P), F(Q), F(R), F(S), F(T), F(U), F(V), F(W),
    F(X), F(Y), F(Z), F(n_lbrack), F(bslash), F(n_rbrack), F(hat), F(under),

    // 0x60-0x6F
    F(n_bquote), F(a), F(b), F(c), F(d), F(e), F(n_f), F(g),
    F(h), F(n_i), F(n_j), F(n_k), F(n_l), F(m), F(n), F(o),

    // 0x70-0x7F
    F(p), F(q), F(r), F(s), F(t), F(u), F(v), F(w),
    F(x), F(y), F(z), F(n_lsquig), F(n_pipe), F(n_rsquig), F(tilde), F(box),
};

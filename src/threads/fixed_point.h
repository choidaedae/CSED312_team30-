#include <stdint.h>

#define F (1<<14)

int int_to_fp(int N){return N*F;};
int fp_to_int(int X){return X/F;};
int fp_to_int_round(int X){return (X>=0)?(X+F/2)/F:(X-F/2);};


int add_fp_fp(int X, int Y){return X+Y;};
int add_fp_int(int X, int N){return X+N*F;};

int sub_fp_fp(int X, int Y){return X-Y;};
int sub_fp_int(int X, int N){return X-N*F;};

int multi_fp_fp(int X, int Y){return ((int64_t)X)*Y/F;};
int multi_fp_int(int X, int N){return X*N;};

int div_fp_fp(int X, int Y){return ((int64_t)X)*F/Y;};
int div_fp_int(int X, int N){return X/N;};
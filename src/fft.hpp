#include <cstring>
#include <cmath>
#include <iostream>

constexpr int nextPow2(int x){
    if(x < 2) std::cout << "WARNING: x must be 2 or more" << std::endl;
    int answer = 2;
    for(;;){
        if(answer >= x) return answer;
        answer *= 2;
    }
}

void fastFourierTransform(
    int N,
    double* timeReal,
    double* timeImag,
    double* freqReal,
    double* freqImag
) {
    double PI = 3.14159265358979323846;
    // double PI = std::numbers::pi_v<double>; // C++20以上ならこちらを使いましょう
    int H = 1; // 行列の行数
    int W = N; // 行列の列数
    memcpy(freqReal, timeReal, sizeof(double) * N);
    memcpy(freqImag, timeImag, sizeof(double) * N);
    double* tempReal = new double[N];
    double* tempImag = new double[N];
    while(H < N){ // 行列は最終的にN行1列になる
        memcpy(tempReal, freqReal, sizeof(double) * N);
        memcpy(tempImag, freqImag, sizeof(double) * N);
        int p; // この段階の基数
        for(p = 2;W % p != 0;p++); // Wを素因数分解
        W /= p; // 次の行列の列数
        double w_N_pN_re = cos(-2.0 * PI / N * N / p); // twiddle factor
        double w_N_pN_im = sin(-2.0 * PI / N * N / p);
        double w_N_W_re = cos(-2.0 * PI / N * W); // もう1つのtwiddle factor
        double w_N_W_im = sin(-2.0 * PI / N * W);
        double w_N_Wi_re = 1.0; // twiddle factorのi乗
        double w_N_Wi_im = 0.0;
        for(int i = 0;i < H;i++){
            for(int j = 0;j < W;j++){
                double w_N_pNm_re = 1.0; // twiddle factorのm乗
                double w_N_pNm_im = 0.0;
                for(int m = 0;m < p;m++){
                    double sumReal = 0.0;
                    double sumImag = 0.0;
                    double w_N_pNm_plus_Wi_re = w_N_pNm_re * w_N_Wi_re - w_N_pNm_im * w_N_Wi_im; // 2つのtwiddle factorをかけあわせる
                    double w_N_pNm_plus_Wi_im = w_N_pNm_re * w_N_Wi_im + w_N_pNm_im * w_N_Wi_re;
                    double w_N_pNmr_plus_Wir_re = 1.0; // かけあわせたtwiddle factorのr乗
                    double w_N_pNmr_plus_Wir_im = 0.0;
                    for(int r = 0;r < p;r++){
                        int index = i * W * p + j + r * W; // indexは(i, j + r * W)成分を指す(もとの行列の列数はW * p)
                        double re = tempReal[index];
                        double im = tempImag[index];
                        sumReal += re * w_N_pNmr_plus_Wir_re - im * w_N_pNmr_plus_Wir_im; // twiddle factorと掛け算して足していく
                        sumImag += re * w_N_pNmr_plus_Wir_im + im * w_N_pNmr_plus_Wir_re;
                        double temp = w_N_pNmr_plus_Wir_re; // twiddle factorのr乗を1つ進める。実部に代入すると虚部の計算に影響するので退避
                        w_N_pNmr_plus_Wir_re = w_N_pNmr_plus_Wir_re * w_N_pNm_plus_Wi_re - w_N_pNmr_plus_Wir_im * w_N_pNm_plus_Wi_im;
                        w_N_pNmr_plus_Wir_im = temp * w_N_pNm_plus_Wi_im + w_N_pNmr_plus_Wir_im * w_N_pNm_plus_Wi_re;
                    }
                    int index = (i + m * H) * W + j; // indexは(i + m * H, j)成分を指す
                    freqReal[index] = sumReal;
                    freqImag[index] = sumImag;
                    double temp = w_N_pNm_re; // twiddle factorのm乗を1つ進める。実部に代入すると虚部の計算に影響するので退避
                    w_N_pNm_re = w_N_pNm_re * w_N_pN_re - w_N_pNm_im * w_N_pN_im;
                    w_N_pNm_im = temp * w_N_pN_im + w_N_pNm_im * w_N_pN_re;
                }
            }
            double temp = w_N_Wi_re; // twiddle factorのi乗を1つ進める。実部に代入すると虚部の計算に影響するので退避
            w_N_Wi_re = w_N_Wi_re * w_N_W_re - w_N_Wi_im * w_N_W_im;
            w_N_Wi_im = temp * w_N_W_im + w_N_Wi_im * w_N_W_re;
        }
        H *= p;
    }
    delete[] tempReal;
    delete[] tempImag;
}

void inverseFastFourierTransform(
    int N,
    double* freqReal,
    double* freqImag,
    double* timeReal,
    double* timeImag
){
    double PI = 3.14159265358979323846;
    // double PI = std::numbers::pi_v<double>; // C++20以上ならこちらを使いましょう
    int H = 1; // 行列の行数
    int W = N; // 行列の列数
    memcpy(timeReal, freqReal, sizeof(double) * N);
    memcpy(timeImag, freqImag, sizeof(double) * N);
    double* tempReal = new double[N];
    double* tempImag = new double[N];
    while(H < N){ // 行列は最終的にN行1列になる
        memcpy(tempReal, timeReal, sizeof(double) * N);
        memcpy(tempImag, timeImag, sizeof(double) * N);
        int p; // この段階の基数
        for(p = 2;W % p != 0;p++); // Wを素因数分解
        W /= p; // 次の行列の列数
        double w_N_pN_re = cos(2.0 * PI / N * N / p); // twiddle factor
        double w_N_pN_im = sin(2.0 * PI / N * N / p);
        double w_N_W_re = cos(2.0 * PI / N * W); // もう1つのtwiddle factor
        double w_N_W_im = sin(2.0 * PI / N * W);
        double w_N_Wi_re = 1.0; // twiddle factorのi乗
        double w_N_Wi_im = 0.0;
        for(int i = 0;i < H;i++){
            for(int j = 0;j < W;j++){
                double w_N_pNm_re = 1.0; // twiddle factorのm乗
                double w_N_pNm_im = 0.0;
                for(int m = 0;m < p;m++){
                    double sumReal = 0.0;
                    double sumImag = 0.0;
                    double w_N_pNm_plus_Wi_re = w_N_pNm_re * w_N_Wi_re - w_N_pNm_im * w_N_Wi_im; // 2つのtwiddle factorをかけあわせる
                    double w_N_pNm_plus_Wi_im = w_N_pNm_re * w_N_Wi_im + w_N_pNm_im * w_N_Wi_re;
                    double w_N_pNmr_plus_Wir_re = 1.0; // かけあわせたtwiddle factorのr乗
                    double w_N_pNmr_plus_Wir_im = 0.0;
                    for(int r = 0;r < p;r++){
                        int index = i * W * p + j + r * W; // indexは(i, j + r * W)成分を指す(もとの行列の列数はW * p)
                        double re = tempReal[index];
                        double im = tempImag[index];
                        sumReal += re * w_N_pNmr_plus_Wir_re - im * w_N_pNmr_plus_Wir_im; // twiddle factorと掛け算して足していく
                        sumImag += re * w_N_pNmr_plus_Wir_im + im * w_N_pNmr_plus_Wir_re;
                        double temp = w_N_pNmr_plus_Wir_re; // twiddle factorのr乗を1つ進める。実部に代入すると虚部の計算に影響するので退避
                        w_N_pNmr_plus_Wir_re = w_N_pNmr_plus_Wir_re * w_N_pNm_plus_Wi_re - w_N_pNmr_plus_Wir_im * w_N_pNm_plus_Wi_im;
                        w_N_pNmr_plus_Wir_im = temp * w_N_pNm_plus_Wi_im + w_N_pNmr_plus_Wir_im * w_N_pNm_plus_Wi_re;
                    }
                    int index = (i + m * H) * W + j; // indexは(i + m * H, j)成分を指す
                    timeReal[index] = sumReal;
                    timeImag[index] = sumImag;
                    double temp = w_N_pNm_re; // twiddle factorのm乗を1つ進める。実部に代入すると虚部の計算に影響するので退避
                    w_N_pNm_re = w_N_pNm_re * w_N_pN_re - w_N_pNm_im * w_N_pN_im;
                    w_N_pNm_im = temp * w_N_pN_im + w_N_pNm_im * w_N_pN_re;
                }
            }
            double temp = w_N_Wi_re; // twiddle factorのi乗を1つ進める。実部に代入すると虚部の計算に影響するので退避
            w_N_Wi_re = w_N_Wi_re * w_N_W_re - w_N_Wi_im * w_N_W_im;
            w_N_Wi_im = temp * w_N_W_im + w_N_Wi_im * w_N_W_re;
        }
        H *= p;
    }
    for(int i = 0;i < N;i++){
        timeReal[i] /= N;
        timeImag[i] /= N;
    }
    delete[] tempReal;
    delete[] tempImag;
}

void convolutionFFT(double* x, int xlen, double* y, int ylen, double* ans){ //直線畳み込み
    int N = xlen + ylen - 1;
    int pow2 = nextPow2(N);
    double* xZeropad = new double[pow2]();
    memcpy(xZeropad, x, sizeof(double) * xlen);
    double* yZeropad = new double[pow2]();
    memcpy(yZeropad, y, sizeof(double) * ylen);
    double* xFreqReal = new double[pow2];
    double* xFreqImag = new double[pow2];
    double* zeros = new double[pow2]();
    fastFourierTransform(pow2, xZeropad, zeros, xFreqReal, xFreqImag);
    double* yFreqReal = new double[pow2];
    double* yFreqImag = new double[pow2];
    fastFourierTransform(pow2, yZeropad, zeros, yFreqReal, yFreqImag);
    double* xFreqYFreqReal = new double[pow2];
    double* xFreqYFreqImag = new double[pow2];
    for(int i = 0;i < pow2;i++){
        xFreqYFreqReal[i] = xFreqReal[i] * yFreqReal[i] - xFreqImag[i] * yFreqImag[i];
        xFreqYFreqImag[i] = xFreqReal[i] * yFreqImag[i] + xFreqImag[i] * yFreqReal[i];
    }
    double* answerPow2 = new double[pow2];
    inverseFastFourierTransform(pow2, xFreqYFreqReal, xFreqYFreqImag, answerPow2, zeros);
    memcpy(ans, answerPow2, sizeof(double)* N);
    delete[] answerPow2;
    delete[] xFreqYFreqImag;
    delete[] xFreqYFreqReal;
    delete[] yFreqImag;
    delete[] yFreqReal;
    delete[] zeros;
    delete[] xFreqImag;
    delete[] xFreqReal;
    delete[] yZeropad;
    delete[] xZeropad;
}

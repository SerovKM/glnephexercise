#include <include/libglnsvpos/func.h>
#include <include/libglnsvpos/glnsvpos.h>
#include <include/libglnsvpos/rungekutta.h>
#include <include/libglnsvpos/structures.h>

using namespace std;

int glnsvpos() {

    // Class Ephemeris
    struct Ephemeris_s Eph;

    // Coordinates
    Eph.X = -8444572.27;
    Eph.Y = -8664957.52;
    Eph.Z = 22466454.10;
    // Velocity
    Eph.VX = 2983.60348;
    Eph.VY = -743.76965;
    Eph.VZ =  832.83615;
    // Acceleration
    Eph.AX = -0.0000028;
    Eph.AY =  0.0000019;
    Eph.AZ = -0.0000019;

    uint16_t Time_year = 2020;
    uint8_t Time_month = 2;
    uint8_t Time_day = 25;
    uint8_t Time_hour = 13;
    uint8_t Time_minutes = 45;
    uint8_t Time_seconds = 18;

    uint16_t year_idx = 0;

    // Time in Gln
    Eph.N4 = ((Time_year - 1996) / 4) + 1;
    while (Eph.N4 > 31) { // Учет 5-битности N4
        Eph.N4 -= 31;
        year_idx++;
    }

    Eph.NT = NT_calc( Eph.N4, Time_year, year_idx, Time_month, Time_day );

    Eph.tb = Time_seconds + Time_minutes*60 + Time_hour*60*60 + 10800;
    if (Eph.tb >= 24*60*60) {
        Eph.tb -= 24*60*60;
        Eph.NT++;
        if (Eph.NT >= 1462) Eph.N4++;
    }

    double GMST = GMST_calc( Eph.N4, Eph.NT );

    struct Ephemeris_s Eph0 = CrdTrnsf2Inertial( Eph, GMST );

    uint32_t tn = Eph.tb; // Текущее время
    double h = 0.5;  // Шаг
    uint32_t Toe = (12+3)*60*60; // Начальное время
    uint32_t Tof = (24+3)*60*60; // Конечное время

    uint32_t N2inc = (Tof - tn + 1) / h; // Количесвио отcчетов для времени большего текущего Eph.tb
    uint32_t N2dec = (tn - Toe + 1) / h; // Количесвио отcчетов для времени меньшего текущего Eph.tb
    uint32_t N = N2inc + N2dec - 1; // Общее число отсчетов

    // Численное интегрирования для времени меньшего текущего Eph.tb
    struct Y_s *Ydec;
    Ydec = new struct Y_s[N2dec];
    Ydec[0].F1 = Eph0.X;
    Ydec[0].F2 = Eph0.Y;
    Ydec[0].F3 = Eph0.Z;
    Ydec[0].F4 = Eph0.VX;
    Ydec[0].F5 = Eph0.VY;
    Ydec[0].F6 = Eph0.VZ;

    RK( N2dec, -h, Ydec);

    // Численное интегрирования для времени большего текущего Eph.tb
    struct Y_s *Yinc;
    Yinc = new struct Y_s[N2inc];
    Yinc[0].F1 = Eph0.X;
    Yinc[0].F2 = Eph0.Y;
    Yinc[0].F3 = Eph0.Z;
    Yinc[0].F4 = Eph0.VX;
    Yinc[0].F5 = Eph0.VY;
    Yinc[0].F6 = Eph0.VZ;

    RK( N2inc, h, Yinc);

    // Формирование выходного массива
    struct Y_s *Yout;
    Yout = new struct Y_s[N];

    uint32_t i;

    for (i = 1; i < N2dec; i++) {
        Yout[i] = Ydec[N2dec-i];
        //cout << "Y[" << i <<"] = " << Yout[i].F1 << "\t" << Yout[i].F2 << "\t" << Yout[i].F3 << "\t" << Yout[i].F4 << "\t" << Yout[i].F5 << "\t" << Yout[i].F6 << endl;
    }
    for (i = 0; i < N2inc; i++) {
        Yout[i+N2dec] = Yinc[i];
        //cout << "Y[" << i+N2dec <<"] = " << Yout[i+N2dec].F1 << "\t" << Yout[i+N2dec].F2 << "\t" << Yout[i+N2dec].F3 << "\t" << Yout[i+N2dec].F4 << "\t" << Yout[i+N2dec].F5 << "\t" << Yout[i+N2dec].F6 << endl;
    }
    delete []Ydec; // Очищение памяти
    delete []Yinc; // Очищение памяти

    // Учет ускорений
    int tau = Toe - tn;
    for (i = 1; i <= N; i++) {

        //cout << "tau = " << tau << endl;

        Yout[i].F1 += Eph0.AX * (tau * tau) / 2;
        Yout[i].F2 += Eph0.AX * (tau * tau) / 2;
        Yout[i].F3 += Eph0.AX * (tau * tau) / 2;

        Yout[i].F4 += Eph0.AX * tau;
        Yout[i].F5 += Eph0.AX * tau;
        Yout[i].F6 += Eph0.AX * tau;

        tau += h;
    }

    FILE *data_out_f;
    if ((data_out_f = fopen("../data_out.txt","wb")) == NULL) {
        perror("Error. Problem with out file\n");
    }
    else {
        for(i = 1; i <= N; i++){
            fprintf(data_out_f,"%e %e %e %e %e %e\n", Yout[i].F1, Yout[i].F2, Yout[i].F3, Yout[i].F4, Yout[i].F5, Yout[i].F6);
        }
    }
    fclose(data_out_f);

    delete []Yout;
}

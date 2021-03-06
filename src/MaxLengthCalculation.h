#ifndef MAXLENGTHCALCULATIONNORMAL_H_
#define MAXLENGTHCALCULATIONNORMAL_H_

#include "Struct.h"

info_t MaxLengthCalculationNormal(MatrixXd matPathFinal, setting_t P, double dMax);

MatrixXd LmaxCalculation1(int n, VectorXd vecL, MatrixXd matDkappa, VectorXd vecLr, VectorXd vecLd);

#endif /* MAXLENGTHCALCULATIONNORMAL_H_ */

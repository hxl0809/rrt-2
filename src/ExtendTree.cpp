#include "ExtendTree.h"

extendTreeR_t ExtendTree_NHC_Sort_OnlyGS_TermCond_Heading(MatrixXd tree, VectorXd vecEndNode, worldLine_t world, setting_t P,
				double dKmax, double c4, double dMax, double dMaxBeta, sigma_t sigma, int iMaxIte, int iINDC)
{
	int iFlag, kk, iTemp, n, idx, flagRet;
	double p, r, theta, Sx, Sy;

	Vector3d vec3RandomPoint;
	VectorXd vecTempDiag, vecIdx, vecP1, vecP2, vecP3, vecNewNode;
	MatrixXd matTemp, matTempSq, matWP, newTree, matdKappa;
	VectorXd vecBeta;

	extendTreeR_t funcReturn;

	iFlag = 0;

	while (iFlag==0) {
		// select a biased random point
		p=Uniform01();

		if ( (iINDC==0 && p<0.1) || (iINDC==1 && p<0.05) ) {
			vec3RandomPoint << vecEndNode(0), vecEndNode(1), vecEndNode(2);
		} else {
			r = sigma.r*Uniform01();
			theta = sigma.theta0 + sigma.theta*quasi_normal_random();

			Sx = sigma.sp(0) + r*cos(theta);
			Sy = sigma.sp(1) + r*sin(theta);
			vec3RandomPoint << Sx, Sy, vecEndNode(2);
		}
		std::cout << "Random Point : " << vec3RandomPoint(0) << " " << vec3RandomPoint(1) << " " << vec3RandomPoint(2) << std::endl;

		// Find node that is closest to random point
		matTemp.resize(tree.rows(),3);

		for (iTemp=0; iTemp<tree.rows(); iTemp++)	{
			matTemp(iTemp,0) = tree(iTemp,0) - vec3RandomPoint(0);
			matTemp(iTemp,1) = tree(iTemp,1) - vec3RandomPoint(1);
			matTemp(iTemp,2) = tree(iTemp,2) - vec3RandomPoint(2);
		}
		matTempSq = matTemp*matTemp.transpose();

		vecTempDiag.resize(matTemp.rows());
		for (iTemp=0; iTemp<matTemp.rows(); iTemp++)
			vecTempDiag(iTemp) = matTempSq(iTemp, iTemp);

		SortVec(vecTempDiag, vecIdx); // vecTempDiag : sorted vector, vecIdx : index of vecTempDiag

		// Modification 3
		if ( vecIdx.rows() > iMaxIte )
			n = iMaxIte;
		else
			n = vecIdx.rows();

		/// Nonholonomic Length Decision
		kk=-1;

		// Modification 4
		if (tree.rows() == 2) {
			vecP1.resize(3); vecP2.resize(3); vecP3.resize(3);
			vecP1 << tree(0,0), tree(0,1), tree(0,2);
			vecP2 << tree(1,0), tree(1,1), tree(1,2);
			vecP3 = vec3RandomPoint;

			matWP.resize(3,3);
			matWP.row(0) = vecP1.segment(0,3).transpose();
			matWP.row(1) = vecP2.segment(0,3).transpose();
			matWP.row(2) = vecP3.segment(0,3).transpose();
			Kappa_Max_Calculation(matWP, P, matdKappa, vecBeta);

			if ( (vecBeta.array().abs()<= dMaxBeta).all() )
				// Method 2 : use the maximum length margin - when there is straight line, there is more margin1
				P.L = 2*dMax;
			else if ( (vecBeta.array().abs() > dMaxBeta).all() ) {
				funcReturn.flag = 0;
				funcReturn.newTree = tree;
				funcReturn.INDC = iINDC;
				funcReturn.sigma = sigma;
				funcReturn.maxIte = iMaxIte;

				return funcReturn;
			}
		} else if (tree.rows() > 2)	{
			for (iTemp=0; iTemp<n; iTemp++)	{
				kk = iTemp;

				if ( tree(vecIdx(iTemp), tree.cols()-1) == 0 )	{
					funcReturn.flag = 0;
					funcReturn.newTree = tree;
					funcReturn.INDC = iINDC;
					funcReturn.sigma = sigma;
					funcReturn.maxIte = iMaxIte;

					return funcReturn;
				}

				vecP2.resize(tree.cols());
				vecP2 = tree.row(vecIdx(iTemp)).transpose();
				vecP1.resize(tree.cols());
				vecP1 = tree.row( vecP2(vecP2.rows()-1)-1 ).transpose();
				vecP3.resize(vec3RandomPoint.rows());
				vecP3 = vec3RandomPoint;

				matWP.resize(3,3);

				matWP.row(0) = vecP1.segment(0,3).transpose();
				matWP.row(1) = vecP2.segment(0,3).transpose();
				matWP.row(2) = vecP3.segment(0,3).transpose();

				Kappa_Max_Calculation(matWP, P, matdKappa, vecBeta);

				if ( (vecBeta.array().abs()<= dMaxBeta).all() )
				{
					// Method 2 : use the maximum length margin - when there is straight line, there is more margin1
					P.L = 2*dMax;
					break;
				} else if ( (vecBeta.array().abs() > dMaxBeta).all() && (iTemp == (n-1) ) ) 	{
					funcReturn.flag = 0;
					funcReturn.newTree = tree;
					funcReturn.INDC = iINDC;
					funcReturn.sigma = sigma;
					funcReturn.maxIte = iMaxIte;

					return funcReturn;
				}
			}
		} //if (tree.rows() == 2)

		if ( kk == -1)
			idx = vecIdx(0);
		else
			idx = vecIdx(kk);

		double cost = tree(idx,4) + P.L;
		Vector3d vecNewPoint = vec3RandomPoint - tree.block(idx,0,1,3).transpose();
		vecNewPoint = tree.block(idx,0,1,3).transpose()+vecNewPoint/vecNewPoint.norm()*P.L;

		if ( kk == -1)
		{
			vecNewNode.resize(vecNewPoint.size()+5);
			vecNewNode << vecNewPoint, 0, cost, P.L, 0, idx+1;
		}
		else
		{
			vecNewNode.resize(vecNewPoint.size()+vecBeta.size()+4);
			vecNewNode << vecNewPoint, 0, cost, P.L, vecBeta.array().abs(), idx+1;
		}

		// check to see if obstacle or random point is reached
		if ( Collision(vecNewNode, tree.row(idx), world, P.SZR, P.SZH, 0) == 0 )
		{
			newTree.resize( tree.rows()+1,tree.cols() );
			newTree.block(0, 0, tree.rows(), tree.cols()) = tree;
			newTree.row(tree.rows()) = vecNewNode.transpose();

			iFlag = 1;
		}
	} // while (iFlag==0)

	// check to see if new node connects directly to end node
	VectorXd vecDiff(3), vecSeg1(3), vecSeg2(3);

	vecSeg1 = vecNewNode.segment(0,3);
	vecSeg2 = vecEndNode.segment(0,3);
	vecDiff = vecSeg1 - vecSeg2;
	double dTerm = vecDiff.norm();

	double psi;
	// 7*dMax
	if ( (dTerm <= 7*dMax) && (iINDC==0) )
	{
		iINDC = 1;

		psi = atan2( vecEndNode(1)-vecNewNode(1), vecEndNode(0)-vecNewNode(0) );
		psi = psi - 2*M_PI*(psi>M_PI);

		sigma.r = dTerm;
		sigma.theta0 = psi;
		sigma.theta = 1.0*M_PI;
		sigma.sp = vecNewNode.segment(0,3);

		iMaxIte = 5;
	}

	if ( iINDC==1 )
	{
		// Method 2
		int end;

		vecP2.resize(vecNewNode.rows());
		vecP2 = vecNewNode;

		end = vecP2.rows();

		if ( vecP2(end-1)==0 )
		{
			vecP1.resize(tree.rows());
			vecP1 = tree.row(0).transpose();
		}
		else
		{
			vecP1.resize(tree.rows());
			vecP1 = tree.row(vecP2(end-1)-1).transpose();
		}

		vecP3.resize(3);
		vecP3 = vecEndNode.segment(0,3);

		matWP.resize(3,3);
		matWP.row(0) = vecP1.segment(0,3);
		matWP.row(1) = vecP2.segment(0,3);
		matWP.row(2) = vecP3.segment(0,3);

		double d_rm;

		Kappa_Max_Calculation(matWP, P, matdKappa, vecBeta);

		d_rm = ( c4*sin(vecP2(end-2)) ) / ( dKmax*cos(vecP2(end-2))*cos(vecP2(end-2)) );

		if ( (2*dMax-d_rm >= 1.0*matdKappa(0,0)) &&
			(Collision(vecNewNode, vecEndNode, world, P.SZR, P.SZH, 0) == 0) &&
			(matdKappa(0,0) <= dTerm) )
		{
			flagRet = 1;
			newTree(newTree.rows()-1,3) = 1;
		}
		else
			flagRet = 0;
	} else
		flagRet = 0;
	// if ( iINDC==1 )

	funcReturn.flag = flagRet;
	funcReturn.newTree = newTree;
	funcReturn.INDC = iINDC;
	funcReturn.sigma = sigma;
	funcReturn.maxIte = iMaxIte;

	return funcReturn;
}

#ifndef __QUANT_NODE_H__
#define __QUANT_NODE_H__
template <	unsigned Abit,unsigned Accbit,unsigned Obit,unsigned ScaleBit, unsigned IntBit>
ap_int<Obit> quant_add(  ap_int<Abit>      in1,
                         ap_int<Accbit>    in2,
					     ap_uint<ScaleBit> rescale, ap_uint<ScaleBit> id_rescale
                             ) {
	const int Max = (1<<(Obit-1))-1;
	const int Min = -(1<<(Obit-1));
	ap_int<Accbit+ScaleBit+1> t_res = (in1*id_rescale + in2 *rescale) >>(ScaleBit-IntBit) ;
	ap_int<Obit> res;

	if (t_res > Max) {
		res = Max;
	} else if (t_res < Min){
		res = Min;
	} else{
		res = t_res;
	}
	//std::cout<<((in1+in2) *rescale)<<" "<<t_res<<" "<< res<<std::endl;
	return res;
}


template <	unsigned Abit,unsigned Obit,unsigned NormWidth,unsigned ScaleBit,unsigned IntBit>
ap_int<Obit> norm_quant(  ap_int<Abit> in,ap_uint<NormWidth> norm_wb) {

	const int Max = (1<<(Obit-1))-1;
	const int Min = -(1<<(Obit-1));

	ap_int<ScaleBit> wt,bias;
	wt.range(ScaleBit-1,0)   = norm_wb.range(ScaleBit-1,0);
	bias.range(ScaleBit-1,0) = norm_wb.range(2*ScaleBit-1,ScaleBit);
	ap_int<Abit+ScaleBit+1> t_res = (in *wt + bias) >> (ScaleBit - IntBit) ;


	ap_int<Obit> res;

	if (t_res > Max) {
		res = Max;
	} else if (t_res < Min){
		res = Min;
	} else{
		res = t_res;
	}

	//std::cout<<t_res<<" "<< res<<std::endl;
	return res;
}


template <	unsigned Ibit,unsigned Obit,unsigned ScaleBit,unsigned IntBit>
ap_int<Obit> quant(  ap_int<Ibit> in,ap_int<ScaleBit> scale) {

	const int Max = (1<<(Obit-1))-1;
	const int Min = -(1<<(Obit-1));

	ap_int<Ibit+ScaleBit+1> t_res = (in *scale) >> (ScaleBit - IntBit) ;
	ap_int<Obit> res;

	if (t_res > Max) {
		res = Max;
	} else if (t_res < Min){
		res = Min;
	} else{
		res = t_res;
	}

	//std::cout<<t_res<<" "<< res<<std::endl;
	return res;
}
#endif

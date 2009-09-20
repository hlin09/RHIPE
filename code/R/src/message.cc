#include <iostream>
#include <rexp.pb.h>
#include "ream.h"

using namespace std;


SEXP rexpress(const char* cmd)
{
  SEXP cmdSexp, cmdexpr, ans = R_NilValue;
  int i;
  ParseStatus status;
  PROTECT(cmdSexp = Rf_allocVector(STRSXP, 1));
  SET_STRING_ELT(cmdSexp, 0, Rf_mkChar(cmd));
  cmdexpr = PROTECT(R_ParseVector(cmdSexp, -1, &status, R_NilValue));
  if (status != PARSE_OK) {
    UNPROTECT(2);
    Rf_error("invalid call: %s", cmd);
    return(R_NilValue);
  }
  for(i = 0; i < Rf_length(cmdexpr); i++)
    ans = Rf_eval(VECTOR_ELT(cmdexpr, i), R_GlobalEnv);
  UNPROTECT(2);
  return(ans);
}

// call (void)Rf_PrintValue(robj) in gdb


SEXP message2rexp(const REXP& rexp){
  SEXP s = R_NilValue;
  int length;
  static int convertLogical[3]={0,1,NA_LOGICAL};
  switch(rexp.rclass()){
  case REXP::NULLTYPE:
    return(R_NilValue);
  case REXP::LOGICAL:
    length = rexp.booleanvalue_size();
    PROTECT(s = Rf_allocVector(LGLSXP,length));
    for (int i = 0; i<length; i++)
      {
  	REXP::RBOOLEAN v = rexp.booleanvalue(i);
  	LOGICAL(s)[i] = convertLogical[1*v];
      }
    UNPROTECT(1);
    break;
  case REXP::INTEGER:
    length = rexp.intvalue_size();
    PROTECT(s = Rf_allocVector(INTSXP,length));
    for (int i = 0; i<length; i++)
      INTEGER(s)[i] = rexp.intvalue(i);
    UNPROTECT(1);
    break;
  case REXP::REAL:
    length = rexp.realvalue_size();
    PROTECT(s = Rf_allocVector(REALSXP,length));
    for (int i = 0; i<length; i++)
      REAL(s)[i] = rexp.realvalue(i);
    UNPROTECT(1);
    break;
  case REXP::RAW:
    {
      const string& r = rexp.rawvalue();
      length = r.size();
      PROTECT(s = Rf_allocVector(RAWSXP,length));
      memcpy(RAW(s),r.data(),length);
      UNPROTECT(1);
      break;
    }
  case REXP::COMPLEX:
    length = rexp.complexvalue_size();
    PROTECT(s = Rf_allocVector(CPLXSXP,length));
    for (int i = 0; i<length; i++){
      COMPLEX(s)[i].r = rexp.complexvalue(i).real();
      COMPLEX(s)[i].i = rexp.complexvalue(i).imag();
    }
    UNPROTECT(1);
    break;
  case REXP::STRING:
    {
      length = rexp.stringvalue_size();
      PROTECT(s = Rf_allocVector(STRSXP,length));
      STRING st;
      for (int i = 0; i<length; i++){
      	st= rexp.stringvalue(i);
      	if (st.isna())
      	  SET_STRING_ELT(s,i,R_NaString);
      	else
      	  SET_STRING_ELT(s,i, Rf_mkChar(st.strval().c_str()));
      }
      UNPROTECT(1);
      break;
    }
  case REXP::LIST:
      length = rexp.rexpvalue_size();
      PROTECT(s = Rf_allocVector(VECSXP,length));
      for (int i = 0; i< length; i++){
	SEXP ik;
	PROTECT(ik = message2rexp(rexp.rexpvalue(i)));
	SET_VECTOR_ELT(s, i,ik );
	UNPROTECT(1);
      }
      UNPROTECT(1);
      break;
  }
  int atlength = rexp.attrname_size();
  if (atlength>0)
    {
      for (int j=0; j<atlength; j++)
  	{
  	  SEXP n=Rf_mkString(rexp.attrname(j).c_str());
  	  Rf_setAttrib(s,n, message2rexp(rexp.attrvalue(j)));
  	}
    }
  return(s);
}


void rexp2message(REXP* rxp,const SEXP robj){
  fill_rexp(rxp,robj);
}



void fill_rexp(REXP* rexp,const SEXP robj){
  
  SEXP xx =   ATTRIB(robj);
  if (xx!=R_NilValue)
    {
      for (SEXP s = ATTRIB(robj); s != R_NilValue; s = CDR(s))
	{
	  // Rf_PrintValue(s);
	  rexp->add_attrname(CHAR(PRINTNAME(TAG(s))));
	  fill_rexp(rexp->add_attrvalue(),
	  	    CAR(s));
	}
    }
  switch(TYPEOF(robj)){
  case LGLSXP:
    rexp->set_rclass(REXP::LOGICAL);
    for (int i = 0; i< LENGTH(robj); i++)
      {
	int d = LOGICAL(robj)[i];
	    switch(d){
	    case 0:
	      rexp->add_booleanvalue(REXP::F);
	      break;
	    case 1:
	      rexp->add_booleanvalue(REXP::T);
	      break;
	    default:
	      rexp->add_booleanvalue(REXP::NA);
	      break;
	    }
      }
    break;
  case INTSXP:
    rexp->set_rclass(REXP::INTEGER);
    for (int i=0; i<LENGTH(robj); i++)
      rexp->add_intvalue(INTEGER(robj)[i]);
    break;
  case REALSXP:
    rexp->set_rclass(REXP::REAL);
    for (int i=0; i<LENGTH(robj); i++)
      rexp->add_realvalue(REAL(robj)[i]);
    break;
  case RAWSXP:{
    rexp->set_rclass(REXP::RAW);
    int l = LENGTH(robj);
    rexp->set_rawvalue((const char*)RAW(robj),l);
    break;
  }
  case CPLXSXP:{
    rexp->set_rclass(REXP::COMPLEX);
    for (int i = 0; i<LENGTH(robj); i++)
      {
	CMPLX *mp = rexp->add_complexvalue();
	mp->set_real(COMPLEX(robj)[i].r);
	mp->set_imag(COMPLEX(robj)[i].i);
      }
    break;
  }
  case NILSXP:{
    rexp->set_rclass(REXP::NULLTYPE);
    break;
  }
  case STRSXP:{
    rexp->set_rclass(REXP::STRING);
    for (int i=0; i<LENGTH(robj); i++){
      STRING* cm = rexp->add_stringvalue();
      if (STRING_ELT(robj,i)==NA_STRING)
	cm->set_isna(true);
      else
	cm->set_strval(CHAR(STRING_ELT(robj,i)));
    }
    break;
  }
  case VECSXP:{
    rexp->set_rclass(REXP::LIST);
    for (int i = 0; i<LENGTH(robj); i++)
  	fill_rexp(rexp->add_rexpvalue(),VECTOR_ELT(robj,i));
    break;
  }
 default:
   rexp->set_rclass(REXP::NULLTYPE);
   break;
  }
 
}
  
  

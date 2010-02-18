/*
  File: register.h
  Author: James Oakley
  Project: Katana
  Date: January 2010
  Description: registers in the patching VM. Exploitation of the fact that Dwarf
               "registers" are a very flexible concept
*/

#include "register.h"
#include "dwarf_instr.h"
#include <assert.h>
#include "patcher/target.h"
#include "symbol.h"
#include <stdio.h>

PoReg readRegFromLEB128(byte* leb,usint* bytesRead)
{
  PoReg result;
  usint numBytes;
  usint numSeptets;
  byte* bytes=decodeLEB128(leb,false,&numBytes,&numSeptets);
  *bytesRead=numSeptets;
  assert(numBytes>0);
  result.type=bytes[0];
  assert(result.type>ERT_PO_START && result.type<ERT_PO_END);
  switch(result.type)
  {
  case ERT_CURR_TARG_NEW:
  case ERT_CURR_TARG_OLD:
    assert(6==numBytes);//todo: diff for 64bit
    result.size=bytes[1];
    memcpy(&result.u.offset,bytes+2,4);
    break;
  case ERT_EXPR:
    assert(5==numBytes);//todo: diff for 64bit
    result.size=0;//not used
    memcpy(&result.u.offset,bytes+1,4);
    break;
  case ERT_NEW_SYM_VAL:
  case ERT_OLD_SYM_VAL:
    assert(5==numBytes);//todo: diff for 64bit
    result.size=0;//not used
    memcpy(&result.u.offset,bytes+1,4);
    break;
  default:
    death("unsupported register type\n");
  }
  return result;
}

//the returned string should be freed
char* strForReg(PoReg reg)
{
  char* buf=zmalloc(128);
  assert(reg.type!=ERT_NONE);
  switch(reg.type)
  {
  case ERT_CURR_TARG_OLD:
    snprintf(buf,128,"{CURR_TARG_OLD,0x%x bytes,0x%x off}",reg.size,reg.u.offset);
    break;
  case ERT_CURR_TARG_NEW:
    snprintf(buf,128,"{CURR_TARG_NEW,0x%x bytes,0x%x off}",reg.size,reg.u.offset);
    break;
  case ERT_EXPR:
    //todo: print out expression
    snprintf(buf,128,"{EXPR,0x%x off",reg.u.offset);
    break;
  case ERT_NEW_SYM_VAL:
    //todo: find symbol name
    snprintf(buf,128,"{NEW_SYM_VAL,idx %i}",reg.u.index);
    break;
  case ERT_OLD_SYM_VAL:
    //todo: find symbol name
    snprintf(buf,128,"{OLD_SYM_VAL,idx %i}",reg.u.index);
    break;
  case ERT_CFA:
    snprintf(buf,128,"{CFA}");
    break;
  default:
    death("unsupported register type\n");
  }
  return buf;
}

void printReg(PoReg reg,FILE* f)
{
  char* str=strForReg(reg);
  fprintf(f,"%s",str);
  free(str);
}

//resolve any register to a value (as distinct from the symbolic form it may be in)
//this may include resolving symbols in elf files, dereferencing
//things in memory, etc
//the result will be written to the result parameter and the number
//of bytes in the result will be returned;
//some values behave differently if they're being assigned than evaluated
//flags determines this behaviour (should be OR'd E_REG_RESOLVE_FLAGS
int resolveRegisterValue(PoReg* reg,SpecialRegsState* state,byte** result,int flags)
{
  //todo: error on value of NEW things if not right hand?
  //perhaps don't even need the rightHand parameter
  addr_t addr=0;
  switch(reg->type)
  {
  case ERT_NONE:
    death("resolveRegisterValue cannot be called on a register of NONE type\n");
    break;
  case ERT_CURR_TARG_NEW:
    addr=state->currAddrNew+reg->u.offset;
    break;
  case ERT_CURR_TARG_OLD:
    addr=state->currAddrOld+reg->u.offset;
    if(flags&ERRF_ASSIGN)
    {
      death("Cannot assign a memory value in the old version of the target\n");
    }
    break;
  case ERT_EXPR:
    //todo: implement these
    death("expressions not supported yet. Poke the developer\n");
    break;
  case ERT_OLD_SYM_VAL:
    if(flags&ERRF_ASSIGN)
    {
      death("Cannot assign a symbol  in the old version of the target\n");
    }
    addr=getSymAddress(state->oldBinaryElf,reg->u.index);
    *result=zmalloc(sizeof(addr_t));
    memcpy(*result,&addr,sizeof(addr_t));
    return sizeof(addr_t);
    break;
  case ERT_NEW_SYM_VAL:
    //todo: implement these
    death("ERT_NEW_SYM_VAL not supported yet. Poke the developer\n");
    break;
  case ERT_CFA:
    //todo: implement these
    death("ERT_CFA not supported yet. Poke the developer\n");
    break;
  default:
    death("unknown register type\n");
  }
  if(addr)
  {
    if(!(flags & ERRF_DEREFERENCE))
    {
      *result=zmalloc(sizeof(addr_t));
      memcpy(*result,&addr,sizeof(addr_t));
      return sizeof(addr_t);
    }
    else
    {
      //need to dereference an address
      *result=zmalloc(reg->size);
      memcpyFromTarget(*result,addr,reg->size);
      return reg->size;
    }
  }
  death("resolveRegisterValue should have returned by now\n");
  return 0;
}

void printRule(PoRegRule rule,int regnum)
{

  char* regStr=NULL;
  regStr=strForReg(rule.regLH);
  switch(rule.type)
  {
  case ERRT_UNDEF:
    printf("%s = Undefined\n",regStr);
    break;
  case ERRT_OFFSET:
    printf("%s = cfa + %i\n",regStr,rule.offset);
    break;
  case ERRT_REGISTER:
    printf("%s = %s\n",regStr,strForReg(rule.regRH));
    break;
  case ERRT_CFA:
    printf("%s = %s + %i\n",regStr,strForReg(rule.regRH),rule.offset);
    break;
  case ERRT_RECURSE_FIXUP:
    printf("%s = recurse fixup with FDE#%i based at %s\n",regStr,rule.index,strForReg(rule.regRH));
    break;
  default:
    death("unknown rule type\n");
  }
}

void printRules(Dictionary* rulesDict,char* tabstr)
{
  PoRegRule** rules=(PoRegRule**)dictValues(rulesDict);
  for(int i=0;rules[i];i++)
  {
    if(rules[i]->type!=ERRT_UNDEF)
    {
      printf("%s",tabstr);
      printRule(*rules[i],i);
    }
  }
}

/*
  File: types.h
  Author: James Oakley
  Project: Katana
  Date: January 10
  Description: types for keeping track of types :)
               Also some function prototypes for comparing
               types and whatnot
*/
#ifndef types_h
#define types_h

#include "dictionary.h"
#include "map.h"

typedef unsigned int uint;
typedef unsigned long addr_t;
typedef uint word_t;

//we keep track of the type information
//from on ELF file in a TypesInfo struct
//note that at the moment we're only worrying about structures
//because they're the only sort of types that can change
//in real world are other things to take into account
//(like the addition and removal of variables, a variable changing its type, etc)
typedef struct
{
  //todo: in the future, may separate things out
  //by compilation unit
  //only dealing with global variable for now
  Dictionary* types; //maps type names to TypeInfo structs
  //List* globalTypesList;//exists to give a unique listing of types, as the dictionary contains typedefs, etc //todo: support this
  Dictionary* globalVars;  /*maps var names to VarInfo structs. Note
                             that this dictionary is the same for all
                             CUs, as in C all global variables must be
                             unique across the program. TODO: add in a
                             CU-local variables section if we ever
                             support any languages that require it */
  Map* parsedDies; //contains global offsets of dwarf dies we've parsed so far
                          //this is necessary because we don't necessarily parse them
                          //in order because a die can refer to a die that comes later
} TypeAndVarInfo;

void freeTypeAndVarInfo(TypeAndVarInfo* tv);

typedef struct
{
  TypeAndVarInfo* tv;
  char* name;
} CompilationUnit;

void freeCompilationUnit(CompilationUnit* cu);

typedef struct
{
  List* compilationUnits;
  List* lastCompilationUnit;
} DwarfInfo;

void freeDwarfInfo(DwarfInfo* di);

typedef enum
{
  TT_STRUCT=1,
  TT_BASE,
  TT_POINTER,
  TT_ARRAY,
  TT_VOID //the void type
} TYPE_TYPE;

//struct to hold information about a type in the target program
//not all members are used by all types (for example, structs use more)
typedef struct
{
  char* name;
  TYPE_TYPE type;
  int length;//overall length in bytes of the type
  bool incomplete;/*if true indicates that this type is in the process
                    of being read. Necessary if for instance a struct
                    has a field which is a pointer to another of the
                    same struct, reading the struct members will
                    reference this type which is only half read
                    itself*/
  //the following members are only applicable to structs
  int numFields;
  char** fields;
  int* fieldLengths;
  char** fieldTypes;
  //the following are relevant only to pointers or arrays
  char* pointedType;//could also be called arrayType
  //the following are relevant only to arrays
  int lowerBound;
  int upperBound;

} TypeInfo;

void freeTypeInfo(TypeInfo* t);

#define FIELD_DELETED -2

//holds information on how to transform one type to another
typedef struct
{
  TypeInfo* from;
  TypeInfo* to;
  //for each field in the 'from' typeinfo,
  //contains the new offset of that field from the start of the new structure
  //this can be used to relocate fields within structures
  //the value FIELD_DELETED indicates that the field is no longer
  //present in the structure
  int* fieldOffsets;
} TypeTransform;

void freeTypeTransform(TypeTransform* t);

typedef struct
{
  char* name;
  TypeInfo* type;
  //location can be gotten from the symbol table
  bool declaration;//true for example for a variable declared extern.
                   //The declaration copy is discarded if a real copy is found
} VarInfo;


void freeVarInfo(VarInfo* v);

//wrapper
void freeVarInfoVoid(void* v);

//structure to hold information about transformation
//necessary to hotpatch types
typedef struct
{
  Dictionary* typeTransformers;//map type name to transformation info
  List* varsToTransform;
  List* varsToTransformEnd;
  //the following two variables are so that
  //we can mmap in page-sized chunks of space
  //at a time and use them for multiple variables
  //they are set at the hot-patching stage
  //rather than at the patch calculation stage
  long addrFreeSpace;
  uint freeSpaceLeft;
} TransformationInfo;

void freeTransformationInfo(TransformationInfo* ti);

#endif
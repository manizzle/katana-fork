/*

  FILE: map.c
  Author: James Oakley
  Data: May, 2009
  Description: structure and prototypes for a general-purpose mapping
  from one type to another

*/


#include "map.h"
#include "hash.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>



//! create an empty map with the given number of buckets
//! hashfunc should be a function to has the key
//! type cmpfunc should be a function to compare two keys, returning
//! 0 iff they are identical
Map* mapCreate(int bucketCnt,unsigned long (*hashfunc_)(void*),int (*cmpfunc_)(void*,void*))
{
  Map* map=zmalloc(sizeof(Map));
  map->hash=zmalloc(bucketCnt*sizeof(MNODE*));
  map->numBuckets=bucketCnt;
  map->size=0;
  map->hashfunc=hashfunc_;
  map->cmpfunc=cmpfunc_;
  return map;
}

//integer comparison function
int intCmp(void* a,void* b)
{
  int ai=*((int*)a);
  int bi=*((int*)b);
  return ai-bi;
}

//passthrough for integer has function in the right form
unsigned long intHash(void* integer)
{
  return hashInt(*((int*)integer));
}

//! helper function for creating a map
//! with int (well, int*), keys
Map* integerMapCreate(int bucketCount)
{
  return mapCreate(bucketCount,intHash,intCmp); 
}

//! clean up a map that's no longer needed
//! If deleteData is non-NULL it is called for each data element
//! If deleteKeys is non-NULL it is called for each key
void mapDelete(Map* map,void (*deleteData)(void*),void (*deleteKey)(void*))
{
  //clean up all items held in the hash table
  MNODE* tmp=map->start;
  while(tmp)
  {
    MNODE* tmp2=tmp->next;
    if(deleteData)
    {
      (*deleteData)(tmp->data);
    }
    if(deleteKey)
    {
      (*deleteKey)(tmp->key);
    }
    free(tmp);
    tmp=tmp2;
  }
  //free the memory for the table itself
  free(map->hash);
  free(map);
}

//!check if a key exists in the map
bool mapExists(const Map* map,void* key)
{
  unsigned int hash=(*(map->hashfunc))(key)%map->numBuckets;
  //printf("check existance key %s with hash of %u\n",key,hash);
  MNODE* node=map->hash[hash];
  if(node)
  {
    do
    {
      if(!(*(map->cmpfunc))(key,node->key))
      {
        return true;
      }
    }while((node=node->next) && hash==node->hash);
  }
  return false;
}

//! add an element to a map
void mapInsert(Map* map,void* key,void* value)
{ 
  assert(!mapExists(map,key));
  MNODE* node=zmalloc(sizeof(MNODE));
  node->data=value;
  node->key=key;

  
  node->hash=(*(map->hashfunc))(key)%map->numBuckets;
  MNODE* parent=map->hash[node->hash];
  if(!parent)
  {
    map->hash[node->hash]=node;
    node->next=0;
    node->prev=map->end;
    if(!map->start)
    {
      map->start=map->end=node;
    }
    else
    {
      map->end->next=node;
      map->end=node;
    }
  }
  else
  {
    while(parent->next && parent->next->hash==node->hash)
    {
      parent=parent->next;
      assert((*(map->cmpfunc))(parent->key,node->key));//don't want to insert the same element into the map again
    }
    if(parent==map->end)
    {
      map->end=node;
    }
    
    
    node->next=parent->next;
    if(node->next)
    {
      node->next->prev=node;
    }
    node->prev=parent;
    parent->next=node;
  }
  map->size++;
}

//! insert an element if the key does not already exist, otherwise
//! update it if deleteData is non-NULL, will be called with an old
//! value being overwritten. deleteKey is used on the older copy of the key in the
//! event that updating an old value
void mapSet(Map* map,void* key,void* value,void (*deleteData)(void*),void (*deleteKey)(void*))
{
  assert(map);
  unsigned int hash=(*(map->hashfunc))(key)%map->numBuckets;
  MNODE* node=map->hash[hash];
  if(node)
  {
    do
    {
      if(!(*(map->cmpfunc))(key,node->key))
      {
        if(deleteData)
        {
          (*deleteData)(node->data);
        }
        if(deleteKey)
        {
          (*deleteKey)(node->key);//not actually using this key for anything
        }
        node->key=key;
        node->data=value;
        return;
      }
    }while((node=node->next) && hash==node->hash);
  }
  
  //since we already have the hash value computed, it would be slightly faster
  //to replicate most of the mapInsert code here ourselves
  //however the speed benefits are minimal so to save my time and effort,
  //we just call mapInsert. In a production/speed-critical system
  //I would either redo the code or construct this and mapInsert out of modular parts
  mapInsert(map,key,value);
}

//! return the data associated with the key. NULL if the key does not exist in the map
void* mapGet(const Map* map,void* key)
{
  assert(map);
  unsigned int hash=(*(map->hashfunc))(key)%map->numBuckets;
  MNODE* node=map->hash[hash];
  if(node)
  {
    do
    {
      if(!(*(map->cmpfunc))(key,node->key))
      {
        return node->data;
      }
    }while((node=node->next) && hash==node->hash);
  }
  return NULL;
}

//! remove the data associated with the key
//! If deleteData is non-NULL it is called for the data element
//! If deleteKeys is non-NULL it is called for the stored key
//! returns false if wasn't able to actually remove anything
bool mapRemove(Map* map,void* key,void (*deleteData)(void*),void (*deleteKey)(void*))
{
  assert(map);
  unsigned int hash=(*(map->hashfunc))(key)%map->numBuckets;
  MNODE* node=map->hash[hash];
  if(!node)
  {
    return false;
  }
  if(deleteData)
  {
    (*deleteData)(node->data);
  }
  if(deleteKey)
  {
    (*deleteKey)(node->key);
  }
  if(node->prev)
  {
    node->prev->next=node->next;
  }
  if(node->next)
  {
    node->next->prev=node->prev;
  }
  if(node==map->start)
  {
    map->start=node->next;
  }
  if(node==map->end)
  {
    map->end=node->prev;
  }
  free(node);
  map->size--;
  return true;
}

//! returns the number of elements in the map
int mapSize(const Map* map)
{
  return map->size;
}

//null-terminated array of all keys in map
//memory for has been malloced, should be freed when you're finished with it
//don't free elements
void** mapKeys(const Map* map)
{
  void** keys=malloc(sizeof(void*)*(map->size+1));
  MNODE* node=map->start;
  for(int i=0;node;node=node->next,i++)
  {
    keys[i]=node->key;
  }
  keys[map->size]=0;
  return keys;
}

//! prints everything in a map, given user supplied print functions
//! for keys and data
void mapPrint(const Map* map,void (*pfuncKey)(void*),void (*pfuncData)(void*))
{
  printf("Map size: %i\nITEMS:\n",map->size);
  MNODE* node=map->start;
  for(;node;node=node->next)
  {
    printf("Key: ");
    (*pfuncKey)(node->key);
    printf(", hash: %u\nValue:",node->hash);
    (*pfuncData)(node->data);
    printf("\n");
  }
}
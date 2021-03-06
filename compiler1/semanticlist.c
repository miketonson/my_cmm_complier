/*
 * =====================================================================================
 *
 *       Filename:  semanticlist.c
 *
 *    Description: type structure defines and hash table defines, hash table oprations for sematic analyze 
 *
 *        Version:  1.0
 *        Created:  04/17/2014 07:40:23 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Guang-Zhi Tang, 
 *   Organization:  
 *
 * =====================================================================================
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "semanticlist.h"

// hash function
unsigned int hash_pjw(char *name)
{
	unsigned int val = 0, i;
	for(; *name; ++name)
	{
		val = (val<<2) + *name;
		if(i = val & ~0x3fff)
			val = (val^(i>>12)) & 0x3fff;
	}
	return val;
}
// init hash table
void initTable()
{
	int i;
	for(i=0; i<table_size; i++)
		hashTable[i] = NULL;

	point *write;
	point *read;
	write = malloc(sizeof(point));
	read = malloc(sizeof(point));
	write->next_point = NULL;
	write->former_point = NULL;
	read->next_point = NULL;
	read->former_point = NULL;

	read->point_type = func_def;
	read->p.func_defPoint = malloc(sizeof(funcDef));
	write->point_type = func_def;
	write->p.func_defPoint = malloc(sizeof(funcDef));

	type *returnType;
	returnType = malloc(sizeof(type));
	returnType->kind = basic;
	returnType->u.basic = 0;

	read->p.func_defPoint->returnType = returnType;
	write->p.func_defPoint->returnType = returnType;
	
	read->p.func_defPoint->name = "read";
	write->p.func_defPoint->name = "write";

	read->p.func_defPoint->funcVarDef = NULL;
	var *varDef;
	varDef = malloc(sizeof(var));
	varDef->name = "i";
	varDef->var_type = returnType;
	varDef->t.funcDef_tail = NULL;
	write->p.func_defPoint->funcVarDef = varDef;

	unsigned int write_num = hash_pjw("write");
	unsigned int read_num = hash_pjw("read");
	hashTable[write_num] = write;
	hashTable[read_num] = read;	
}
// init layer stack
void initStack()
{
	nowLayer = NULL;
}
// insert layer stack point
void pushLayerStack(int layer)
{
	varStack *thisLayer;
	thisLayer = malloc(sizeof(varStack));
	thisLayer->upperLayer = nowLayer;
	thisLayer->layerPoint = NULL;
	thisLayer->layer = layer;
	nowLayer = thisLayer;
}
// add layer var to the layer point
void addPointLayer(point *addPoint)
{
	if(nowLayer->layerPoint == NULL)
	{
		nowLayer->layerPoint = addPoint;
		return;
	}
	point *tempPoint;
	tempPoint = nowLayer->layerPoint;
	while((tempPoint->point_type == var_def && tempPoint->p.var_defPoint.next_varPoint != NULL) || (tempPoint->point_type == struct_dec && tempPoint->p.struct_decPoint.next_varPoint != NULL))
	{
		if(tempPoint->point_type == var_def)
		{
			tempPoint = tempPoint->p.var_defPoint.next_varPoint;
		}
		else if(tempPoint->point_type == struct_dec)
		{
			tempPoint = tempPoint->p.struct_decPoint.next_varPoint;
		}
		else
		{
			printf("insert stack point error: 1\n");
			return;
		}
	}
	if(tempPoint->point_type == var_def)
	{
		tempPoint->p.var_defPoint.next_varPoint = addPoint;
		return;
	}
	else if(tempPoint->point_type == struct_dec)
	{
		tempPoint->p.struct_decPoint.next_varPoint = addPoint;
		return;
	}
	else
	{
		printf("insert stack point error: 2\n");
		return;
	}
}
// add function def & dec point to hash table
int addFuncTable(point *funcPoint)
{
	unsigned int value;
	if(funcPoint->point_type == func_dec)
	{
		value = hash_pjw(funcPoint->p.func_decPoint->name);
	}
	else if(funcPoint->point_type == func_def)
	{
		value = hash_pjw(funcPoint->p.func_defPoint->name);
	}
	else
	{
		printf("insert func hash error: not a func type\n");
		return -1;// error
	}
	if(hashTable[value] == NULL)
	{
		hashTable[value] = funcPoint;
		return 0; // return means success
	}
	point *nextPoint;
	nextPoint = hashTable[value];
	while(nextPoint != NULL)
	{
		switch(funcPoint->point_type)
		{
			case func_def:
			{
				if(nextPoint->point_type == func_def && strcmp(nextPoint->p.func_defPoint->name, funcPoint->p.func_defPoint->name) == 0)
				{
					return 1;// multi def
				}
				if(nextPoint->point_type == func_dec && strcmp(nextPoint->p.func_decPoint->name, funcPoint->p.func_defPoint->name) == 0)
				{
					switch(isSameFunc(nextPoint, funcPoint))
					{
						case 0:
						{
							break;
						}
						case 1:
						{
							funcPoint->p.func_defPoint = nextPoint->p.func_decPoint;
							hashTable[value]->former_point = funcPoint;
							funcPoint->next_point = hashTable[value];
							hashTable[value] = funcPoint;
							return 2;// func def and dec not same
						}
					}
				}
				break;
			}
			case func_dec:
			{
				if((nextPoint->point_type == func_def && strcmp(nextPoint->p.func_defPoint->name, funcPoint->p.func_decPoint->name) == 0) || (nextPoint->point_type == func_dec && strcmp(nextPoint->p.func_decPoint->name, funcPoint->p.func_decPoint->name) == 0))
				{
					switch(isSameFunc(nextPoint, funcPoint))
					{
						case 0:
							{
								break;
							}
						case 1:
							{
								return 2;// func def dec dec not same
							}
					}
				}	
				break;
			}
		}
		nextPoint = nextPoint->next_point;
	}
	hashTable[value]->former_point = funcPoint;
	funcPoint->next_point = hashTable[value];
	hashTable[value] = funcPoint;
	return 0; // return means success
}
// if it define a same function
int isSameFunc(point *firPoint, point *secPoint)
{
	funcDef *firFunc;
	funcDef *secFunc;
	switch(firPoint->point_type)
	{
		case func_dec:
			{
				firFunc = firPoint->p.func_decPoint;
				break;
			}
		case func_def:
			{
				firFunc = firPoint->p.func_defPoint;
				break;
			}
	}
	switch(secPoint->point_type)
	{
		case func_dec:
			{
				secFunc = secPoint->p.func_decPoint;
				break;
			}
		case func_def:
			{
				secFunc = secPoint->p.func_defPoint;
				break;
			}
	}
	switch(isSameType(firFunc->returnType, secFunc->returnType, 0))
	{
		case 0:
			break;
		case 1:
			{
				return 1; // type not the same
			}
	}
	var *firTail;
	var *secTail;
	firTail = firFunc->funcVarDef;
	secTail = secFunc->funcVarDef;
	if(firTail == NULL && secTail == NULL)
	{
		return 0; // same func have no var
	}
	while(firTail != NULL || secTail != NULL)
	{
		if(firTail == NULL && secTail != NULL)
		{
			return 1; // not same length var
		}
		if(firTail != NULL && secTail == NULL)
		{
			return 1; // not same length var
		}
		if(isSameType(firTail->var_type, secTail->var_type, 0) != 0)
		{
			return 1; // have var not same type
		}
		firTail = firTail->t.funcDef_tail;
		secTail = secTail->t.funcDef_tail;
	}
	return 0;// same
}
// if it is the same type
int isSameType(type *firType, type *secType, int nowLayerNum)
{
	if(firType->kind != secType->kind)
	{
		return 1;// not same type kind
	}
	switch(firType->kind)
	{
		case basic:
			{
				if(firType->u.basic == secType->u.basic)
				{
					return 0; // same basic type
				}
				else
				{
					return 1; // different basic type
				}
				break;
			}
		case array:
			{
				int isNext = isSameType(firType->u.array.elem, secType->u.array.elem, nowLayerNum);
				if(isNext == 0 /*&& firType->u.array.size == secType->u.array.size*/)
				{
					return 0; // same array type
				}
				else
				{
					return 1; // different array type
				}
				break;
			}
		case structure:
			{
				if((firType->u.stru.struct_name != NULL && secType->u.stru.struct_name != NULL) && strcmp(firType->u.stru.struct_name, secType->u.stru.struct_name) == 0 && (firType->u.stru.structure == NULL && secType->u.stru.structure == NULL))
				{
					return 0; // same struct type
				}
				var *firTail = firType->u.stru.structure;
				var *secTail = secType->u.stru.structure;
				if(firTail == NULL || secTail == NULL)
				{
					char *firName = firType->u.stru.struct_name;
					char *secName = secType->u.stru.struct_name;
					point *firPoint = findPoint(firName, 3, nowLayerNum);
					point *secPoint = findPoint(secName, 3, nowLayerNum);
					int r = isSameType(firPoint->p.struct_decPoint.struct_decP, secPoint->p.struct_decPoint.struct_decP, nowLayerNum);
					return r; // not same type
				}
				while(firTail != NULL || secTail != NULL)
				{
					if(firTail == NULL && secTail != NULL)
					{
						return 1; // not same length
					}
					if(firTail != NULL && secTail == NULL)
					{
						return 1; // not same length
					}
					if(isSameType(firTail->var_type, secTail->var_type, nowLayerNum + 1) != 0)
					{
						return 1; // not same area type
					}
					firTail = firTail->t.struct_tail;
					secTail = secTail->t.struct_tail;
				}
				return 0;
				break;
			}
	}
}
// add variable point to hash table
int addVarTable(point *varPoint)
{
	unsigned int value;
	if(varPoint->point_type == var_def)
	{
		value = hash_pjw(varPoint->p.var_defPoint.var_defP->name);
	}
	else if(varPoint->point_type == struct_dec)
	{
		value = hash_pjw(varPoint->p.struct_decPoint.struct_decP->u.stru.struct_name);
	}
	else
	{
		printf("var inseat error: not a var or struct\n");
		return -1;
	}
	if(hashTable[value] == NULL)
	{
		hashTable[value] = varPoint;
		addPointLayer(varPoint);
		return 0; // return means success
	}
	point *nextPoint;
	nextPoint = hashTable[value];
	while(nextPoint != NULL)
	{
		switch(nextPoint->point_type)
		{
			case var_def:
				{
					switch(varPoint->point_type)
					{
						case var_def:
							{
								if(strcmp(nextPoint->p.var_defPoint.var_defP->name, varPoint->p.var_defPoint.var_defP->name) == 0 && varPoint->p.var_defPoint.layer == nextPoint->p.var_defPoint.layer)
								{
									return 1; // def and def multi
								}
								break;
							}
						case struct_dec:
							{
								if(strcmp(nextPoint->p.var_defPoint.var_defP->name, varPoint->p.struct_decPoint.struct_decP->u.stru.struct_name) == 0 && varPoint->p.struct_decPoint.layer == nextPoint->p.var_defPoint.layer)
								{
									return 2; // struct and def multi
								}
								break;
							}
						default:
							{
								return -1;
								break;
							}
					}
					break;
				}
			case struct_dec:
				{
					switch(varPoint->point_type)
					{
						case var_def:
							{
								if(strcmp(nextPoint->p.struct_decPoint.struct_decP->u.stru.struct_name, varPoint->p.var_defPoint.var_defP->name) == 0 && varPoint->p.var_defPoint.layer == nextPoint->p.struct_decPoint.layer)
								{
									return 1; // def and struct multi
								}
								break;
							}
						case struct_dec:
							{
								if(strcmp(nextPoint->p.struct_decPoint.struct_decP->u.stru.struct_name, varPoint->p.struct_decPoint.struct_decP->u.stru.struct_name) == 0 && varPoint->p.struct_decPoint.layer == nextPoint->p.struct_decPoint.layer)
								{
									return 2; // struct and struct multi
								}
								break;
							}
						default:
							{
								return -1;
								break;
							}
					}
					break;
				}
			default:
				{
					break;
				}
		}
		nextPoint = nextPoint->next_point;
	}
	hashTable[value]->former_point = varPoint;
	varPoint->next_point = hashTable[value];
	hashTable[value] = varPoint;
	addPointLayer(varPoint);
	return 0; // return means success
}
// delete a layer from the layer stack and hash table
void pullLayerStack()
{
	// pull from layer stack
	varStack *firLayer;
	firLayer = nowLayer;
	nowLayer = firLayer->upperLayer;
	// delete from hash table
	point *tempPoint;
	tempPoint = firLayer->layerPoint;
	while(tempPoint != NULL)
	{
		if(tempPoint->next_point == NULL && tempPoint->former_point == NULL)
		{
		
			unsigned int num;
			point *tempNew;
			switch(tempPoint->point_type)
			{
				case var_def:
					{
						num = hash_pjw(tempPoint->p.var_defPoint.var_defP->name);
						hashTable[num] = NULL;	
						tempNew = tempPoint->p.var_defPoint.next_varPoint;
						break;
					}
				case struct_dec:
					{
						num = hash_pjw(tempPoint->p.struct_decPoint.struct_decP->u.stru.struct_name);
						hashTable[num] = NULL;
						tempNew = tempPoint->p.struct_decPoint.next_varPoint;
						break;
					}
				default:
					{
						printf("pull error type\n");
						break;
					}
			}	
			free(tempPoint);
			tempPoint = tempNew;
		}
		else if(tempPoint->next_point == NULL && tempPoint->former_point != NULL)
		{
			point *tempNew;
			point *tempNextNew;
			tempNew = tempPoint->former_point;
			tempNew->next_point = NULL;
			switch(tempPoint->point_type)
			{
				case var_def:
					{
						tempNextNew = tempPoint->p.var_defPoint.next_varPoint;
						break;
					}
				case struct_dec:
					{
						tempNextNew = tempPoint->p.struct_decPoint.next_varPoint;
						break;
					}
				default:
					{
						printf("pull error type\n");
						break;
					}
			}
			free(tempPoint);
			tempPoint = tempNextNew;
		}
		else if(tempPoint->next_point != NULL && tempPoint->former_point == NULL)
		{
			unsigned int num;
			point *tempNew;
			switch(tempPoint->point_type)
			{
				case var_def:
					{
						num = hash_pjw(tempPoint->p.var_defPoint.var_defP->name);
						hashTable[num] = tempPoint->next_point;
						tempNew = tempPoint->p.var_defPoint.next_varPoint;
						break;
					}
				case struct_dec:
					{
						num = hash_pjw(tempPoint->p.struct_decPoint.struct_decP->u.stru.struct_name);
						hashTable[num] = tempPoint->next_point;
						tempNew = tempPoint->p.struct_decPoint.next_varPoint;
						break;
					}
				default:
					{
						printf("pull error type\n");
						break;
					}
			}
			free(tempPoint);
			tempPoint = tempNew;
		}
		else
		{
			point *formerPoint;
			point *nextPoint;
			point *tempNew;
			formerPoint = tempPoint->former_point;
			nextPoint = tempPoint->next_point;
			formerPoint->next_point = nextPoint;
			nextPoint->former_point = formerPoint;
			switch(tempPoint->point_type)
			{
				case var_def:
					{
						tempNew = tempPoint->p.var_defPoint.next_varPoint;
						break;
					}
				case struct_dec:
					{
						tempNew = tempPoint->p.struct_decPoint.next_varPoint;
						break;
					}
				default:
					{
						printf("pull error type\n");
						break;
					}
			}
			free(tempPoint);
			tempPoint = tempNew;
		}
	}
	free(firLayer);
}
// find point in hash table, type 0 for fun_dec, 1 for fun_def, 2 for var_def, 3 for struct_dec
point * findPoint(char *name, int type, int layer)
{
	point *returnPoint;
	unsigned int num = hash_pjw(name);
	returnPoint = hashTable[num];
	while(returnPoint != NULL)
	{
		switch(type)
		{
			case 0:
				{
					if(returnPoint->point_type == func_dec)
					{
						if(strcmp(name, returnPoint->p.func_decPoint->name) == 0)
						{
							return returnPoint;
						}
					}
					returnPoint = returnPoint->next_point;
					break;
				}
			case 1:
				{
					if(returnPoint->point_type == func_def)
					{
						if(strcmp(name, returnPoint->p.func_defPoint->name) == 0)
						{
							return returnPoint;
						}
					}
					returnPoint = returnPoint->next_point;
					break;
				}
			case 2:
				{
					if(returnPoint->point_type == var_def)
					{
						if(strcmp(name, returnPoint->p.var_defPoint.var_defP->name) == 0 && layer >= returnPoint->p.var_defPoint.layer)
						{
							return returnPoint;
						}
					}
					returnPoint = returnPoint->next_point;
					break;
				}
			case 3:
				{
					if(returnPoint->point_type == struct_dec)
					{
						if(strcmp(name, returnPoint->p.struct_decPoint.struct_decP->u.stru.struct_name) == 0 && layer>= returnPoint->p.struct_decPoint.layer)
						{
							return returnPoint;
						}
					}
					returnPoint = returnPoint->next_point;
					break;
				}
			default:
				{
					printf("error type input\n");
					break;
				}
		}
	}
	return returnPoint;
}

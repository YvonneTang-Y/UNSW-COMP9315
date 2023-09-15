/*
 * src/tutorial/gcoord.c
 *
 ******************************************************************************
  This file contains routines that can be bound to a Postgres backend and
  called by the backend in the process of processing queries.  The calling
  format for these routines is dictated by Postgres architecture.
******************************************************************************/

#include "postgres.h"
#include "fmgr.h"
#include "libpq/pqformat.h"		/* needed for send/recv functions */
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <regex.h>
#include <ctype.h>

PG_MODULE_MAGIC;

typedef struct GeoCoord
{
	int32 	length;
	float	latitude;
	char	latitude_flag;
	float	longitude;
	char	longitude_flag;
	char 	location_name[FLEXIBLE_ARRAY_MEMBER];

}GeoCoord;

// check whether input is valid using regex pattern
bool valid(char *input, char *pattern);

bool valid(char *input, char *pattern)
{
	regex_t regex;
	bool 	valid_flag = false;
	if (regcomp(&regex, pattern, REG_EXTENDED |REG_ICASE |REG_NOSUB) == 0)
	{
		if (regexec(&regex, input, 0, NULL, 0) == 0)
		{
			valid_flag = true;
		}
	}
	regfree(&regex);
	return valid_flag;
}


/*****************************************************************************
 * Input/Output functions
 *****************************************************************************/

PG_FUNCTION_INFO_V1(gcoord_in);

Datum
gcoord_in(PG_FUNCTION_ARGS)
{
	char	   	*str = PG_GETARG_CSTRING(0);
	char		*location_name;
	float		latitude;
	char		latitude_flag;
	float		longitude;
	char		longitude_flag;

	char		*latitude_str;
	char		*longitude_str;
	GeoCoord   	*result;
	char 		*token;
	char		*str_copy = (char *) malloc(strlen(str) + 1);

	if (!valid(str,"^([A-Za-z][A-Za-z ]*[A-Za-z],(([1-8]?[0-9](\\.[0-9]{1,4})?)|(90(\\.0{1,4})?))°[NS][, ](([0-9]{1,2}(\\.[0-9]{1,4})?)|(1[0-7][0-9](\\.[0-9]{1,4})?)|180(\\.0{1,4})?)°[WE])|([A-Za-z][A-Za-z ]*[A-Za-z],(([0-9]{1,2}(\\.[0-9]{1,4})?)|(1[0-7][0-9](\\.[0-9]{1,4})?)|180(\\.0{1,4})?)°[WE][, ](([1-8]?[0-9](\\.[0-9]{1,4})?)|(90(\\.0{1,4})?))°[NS])$"))
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
				 errmsg("invalid input syntax for type %s: \"%s\"",
						"GeoCoord", str)));
	
	strcpy(str_copy, str);
	token = strtok(str_copy, ", ");

	location_name = (char *) malloc(strlen(token) + 1);
	strcpy(location_name, token);
	// convert to canonical form
	for(int i = 0; location_name[i]; i++)
	{
		location_name[i] = tolower(location_name[i]);
	}

	while(token != NULL)
	{
		if (valid(token, "^(([1-8]?[0-9](\\.[0-9]{1,4})?)|(90(\\.0{1,4})?))°[NS]$"))
		{
			latitude_str = (char *) malloc(strlen(token) + 1);
			strcpy(latitude_str, token);
		}

		else if (valid(token, "^(([0-9]{1,2}(\\.[0-9]{1,4})?)|(1[0-7][0-9](\\.[0-9]{1,4})?)|180(\\.0{1,4})?)°[WE]$"))
		{
			longitude_str = (char *) malloc(strlen(token) + 1);
			strcpy(longitude_str, token);	
		}

		token = strtok(NULL, ",");
	}

	token = strtok(latitude_str, "°");
	latitude = atof(token);
	token = strtok(NULL, "°");
	latitude_flag = token[0];

	token = strtok(longitude_str, "°");
	longitude = atof(token);
	token = strtok(NULL, "°");
	longitude_flag = token[0];

	result = (GeoCoord *) palloc(VARHDRSZ + sizeof(GeoCoord *) + strlen(location_name) + 1);
	// need to give value to the three varibles
	SET_VARSIZE(result, VARHDRSZ + sizeof(GeoCoord *) + strlen(location_name) + 1);
	memcpy(result->location_name, location_name, strlen(location_name) + 1);
	result->latitude = latitude;
	result->latitude_flag = latitude_flag;
	result->longitude = longitude;
	result->longitude_flag = longitude_flag;

	free(location_name);
	free(latitude_str);
	free(longitude_str);
	free(str_copy);
	PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(gcoord_out);

Datum
gcoord_out(PG_FUNCTION_ARGS)
{
	GeoCoord    *gcoord = (GeoCoord *) PG_GETARG_POINTER(0);
	char	   *result;

	// result = psprintf("%s", gcoord->data);
	result = psprintf("%s,%.4f°%c,%.4f°%c", gcoord->location_name, gcoord->latitude, gcoord->latitude_flag, gcoord->longitude, gcoord->longitude_flag);
	PG_RETURN_CSTRING(result);
}

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
#include "access/hash.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <regex.h>
#include <ctype.h>

PG_MODULE_MAGIC;

typedef struct GeoCoord
{
	int32 	length;
	char 	data[FLEXIBLE_ARRAY_MEMBER];
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
	char		*latitude;
	char		*longitude;
	GeoCoord   	*result;
	char 		*token;
	char		*str_copy = (char *) palloc(strlen(str) + 1);

	// check whether input string is valid
	if (!valid(str,"^([A-Za-z][A-Za-z ]*[A-Za-z],(([1-8]?[0-9](\\.[0-9]{0,4})?)|(90(\\.0{0,4})?))°[NS][, ](([0-9]{1,2}(\\.[0-9]{0,4})?)|(1[0-7][0-9](\\.[0-9]{0,4})?)|180(\\.0{0,4})?)°[WE])|([A-Za-z][A-Za-z ]*[A-Za-z],(([0-9]{1,2}(\\.[0-9]{0,4})?)|(1[0-7][0-9](\\.[0-9]{0,4})?)|180(\\.0{0,4})?)°[WE][, ](([1-8]?[0-9](\\.[0-9]{0,4})?)|(90(\\.0{0,4})?))°[NS])$"))
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
				 errmsg("invalid input syntax for type %s: \"%s\"",
						"GeoCoord", str)));
	
	strcpy(str_copy, str);
	token = strtok(str_copy, ",");

	location_name = (char *) palloc(strlen(token) + 1);
	strcpy(location_name, token);

	if (!valid(location_name,"^[A-Za-z][A-Za-z ]*[A-Za-z]$"))
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
				 errmsg("invalid input syntax for type %s: \"%s\"",
						"GeoCoord", str)));

	// convert to canonical form: locationName,latitude,longitude
	for(int i = 0; location_name[i]; i++)
	{
		location_name[i] = tolower(location_name[i]);
	}

	while(token != NULL)
	{
		if (valid(token, "^(([1-8]?[0-9](\\.[0-9]{0,4})?)|(90(\\.0{0,4})?))°[NS]$"))
		{
			latitude = (char *) palloc(strlen(token) + 1);
			strcpy(latitude, token);
		}

		else if (valid(token, "^(([0-9]{1,2}(\\.[0-9]{0,4})?)|(1[0-7][0-9](\\.[0-9]{0,4})?)|180(\\.0{0,4})?)°[WE]$"))
		{
			longitude = (char *) palloc(strlen(token) + 1);
			strcpy(longitude, token);	
		}

		token = strtok(NULL, ", ");
	}
	sprintf(str_copy, "%s,%s,%s", location_name, latitude, longitude);

	// allocate memory and set length
	result = (GeoCoord *) palloc(VARHDRSZ + strlen(str_copy) + 1);
	SET_VARSIZE(result, VARHDRSZ + strlen(str_copy) + 1);
	memcpy(result->data, str_copy, strlen(str_copy) + 1);

	PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(gcoord_out);

Datum
gcoord_out(PG_FUNCTION_ARGS)
{
	GeoCoord    *gcoord = (GeoCoord *) PG_GETARG_POINTER(0);
	char	   *result;

	result = psprintf("%s", gcoord->data);

	PG_RETURN_CSTRING(result);
}

/*****************************************************************************
 * Binary Input/Output functions
 *
 * These are optional.
 *****************************************************************************/

PG_FUNCTION_INFO_V1(gcoord_recv);

Datum
gcoord_recv(PG_FUNCTION_ARGS)
{
	StringInfo	buf = (StringInfo) PG_GETARG_POINTER(0);
	GeoCoord    *result;
	const char 	*data = pq_getmsgstring(buf);
	int			length = strlen(data) + 1;

	result = (GeoCoord *) palloc(VARHDRSZ + length);
	memcpy(result->data, data, length);
	PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(gcoord_send);

Datum
gcoord_send(PG_FUNCTION_ARGS)
{
	GeoCoord    	*gcoord = (GeoCoord *) PG_GETARG_POINTER(0);
	StringInfoData 	buf;

	pq_begintypsend(&buf);
	pq_sendstring(&buf, gcoord->data);
	PG_RETURN_BYTEA_P(pq_endtypsend(&buf));
}

/*****************************************************************************
 * New Operators
 *
 * A practical GeoCoord datatype would provide much more than this, of course.
 *****************************************************************************/


/*****************************************************************************
 * create several function to get
 *		location name
 *		latitude value
 *		latitude direction
 *		longitude value
 *		longitude direction
 *	
 * and create function to convert from double to DMS.
 *****************************************************************************/
char *get_location(char *data);

char *get_location(char *data)
{
	char *str = (char *) malloc(strlen(data) + 1);
	char *token;
	strcpy(str, data);
	token = strtok(str, ",");
	return token;
}

double get_latitude(char *data);

double get_latitude(char *data)
{
	char *str = (char *) malloc(strlen(data) + 1);
	char *token;
	double latitude;
	strcpy(str, data);
	token = strtok(str, ",");
	token = strtok(NULL, "°");
	latitude = atof(token);
	free(str);
	return latitude;
}

char get_latitude_flag(char *data);

char get_latitude_flag(char *data)
{
	char *str = (char *) malloc(strlen(data) + 1);
	char *token;
	char latitude_flag;
	strcpy(str, data);
	token = strtok(str, ",");
	token = strtok(NULL, "°,");
	token = strtok(NULL, "°,");
	latitude_flag = token[0];
	free(str);
	return latitude_flag;
}

double get_longitude(char *data);

double get_longitude(char *data)
{
	char *str = (char *) malloc(strlen(data) + 1);
	char *token;
	double longitude;
	strcpy(str, data);
	token = strtok(str, ",");
	token = strtok(NULL, ",");
	token = strtok(NULL, "°,");
	longitude = atof(token);
	free(str);
	return longitude;
}

char get_longitude_flag(char *data);

char get_longitude_flag(char *data)
{
	char *str = (char *) malloc(strlen(data) + 1);
	char *token;
	char longitude_flag;
	strcpy(str, data);
	token = strtok(str, ",");
	token = strtok(NULL, ",");
	token = strtok(NULL, "°,");
	token = strtok(NULL, "°,");
	longitude_flag = token[0];
	free(str);
	return longitude_flag;
}

char *convert(double decimal);

char *convert(double decimal)
{
	int		d, m;
	double	s;
	char 	*dms = (char *) malloc(15);
	char	*s_str = (char *) malloc(7);

	d = (int) decimal;
	sprintf(s_str, "%.4f", decimal - d);
	m = (int)(atoi(s_str+2) * 60 /10000);
	/* 	when convert string to float, there will some small error
	* 	such as convert '122.42' to float, the result will be 122.419998
	* 	therefore, s can't be directly converted to int*/

	/* 	some modification:
		I use double instead of float
		but I think using double may encouter the same problem about accuracy
		since the value is no more than 4 decimal places, this method is still useful
	*/
	s = (int)((3600 * (atoi(s_str+2)) - 60 * m * 10000) / 10000);
	if (s != 0)
		sprintf(dms, "%d°%d'%.0f\"", d, m, s);
	else if (m != 0)
		sprintf(dms, "%d°%d'", d, m);
	else
		sprintf(dms, "%d°", d);
	free(s_str);
	return dms;
}


PG_FUNCTION_INFO_V1(convert_dms);

Datum
convert_dms(PG_FUNCTION_ARGS)
{
	GeoCoord    *gcoord = (GeoCoord *) PG_GETARG_POINTER(0);
	char		*location_name = get_location(gcoord->data);
	char 		*latitude_dms = convert(get_latitude(gcoord->data));
	char 		latitude_flag = get_latitude_flag(gcoord->data);
	char 		*longitude_dms = convert(get_longitude(gcoord->data));
	char 		longitude_flag = get_longitude_flag(gcoord->data);
	char		*result;
	// format result
	result = psprintf("%s,%s%c,%s%c", location_name, latitude_dms, latitude_flag, longitude_dms, longitude_flag);
	free(location_name);
	free(latitude_dms);
	free(longitude_dms);
	PG_RETURN_CSTRING(result);
}


/*****************************************************************************
 * Operator class for defining B-tree index
 *
 * It's essential that the comparison operators and support function for a
 * B-tree index opclass always agree on the relative ordering of any two
 * data values.  Experience has shown that it's depressingly easy to write
 * unintentionally inconsistent functions.  One way to reduce the odds of
 * making a mistake is to make all the functions simple wrappers around
 * an internal three-way-comparison function, as we do here.
 *****************************************************************************/

static int
gcoord_cmp_internal(GeoCoord * a, GeoCoord * b)
{
	// compare the two string first, if they are totally same
	if (strcmp(a->data, b->data) == 0)
		return 0;

	// latitude: the bigger one is closer to equator(zero) and N > S
	if (get_latitude(a->data) < get_latitude(b->data))
		return 1;
	if (get_latitude(a->data) > get_latitude(b->data))
		return -1;
	if (get_latitude_flag(a->data) != get_latitude_flag(b->data))
	{
		if (get_latitude_flag(a->data) == 'N')
			return 1;
		else
			return -1;
	}

	// longitude: the bigger one is closer to prime meridian(zero) and E > W
	if (get_longitude(a->data) < get_longitude(b->data))
		return 1;
	if (get_longitude(a->data) > get_longitude(b->data))
		return -1;
	if (get_longitude_flag(a->data) != get_longitude_flag(b->data))
	{
		if (get_longitude_flag(a->data) == 'E')
			return 1;
		else
			return -1;
	}

	char *location_a = get_location(a->data);
	char *location_b = get_location(b->data);

	// compare the locationName lexically 
	int location_flag = strcmp(location_a, location_b);
	free(location_a);
	free(location_b);
	return location_flag;
}

// this function is used to compare if the two location is in the same zone
static int
gcoord_zone_internal(GeoCoord * a, GeoCoord * b)
{
	if (get_longitude_flag(a->data) != get_longitude_flag(b->data))
		return -1;
	if ((int) (get_longitude(a->data) / 15) == (int) (get_longitude(b->data) / 15))
		return 1;
	return -1;
}

// create operators
PG_FUNCTION_INFO_V1(gcoord_lt);

Datum
gcoord_lt(PG_FUNCTION_ARGS)
{
	GeoCoord    *a = (GeoCoord *) PG_GETARG_POINTER(0);
	GeoCoord    *b = (GeoCoord *) PG_GETARG_POINTER(1);

	PG_RETURN_BOOL(gcoord_cmp_internal(a, b) < 0);
}

PG_FUNCTION_INFO_V1(gcoord_le);

Datum
gcoord_le(PG_FUNCTION_ARGS)
{
	GeoCoord    *a = (GeoCoord *) PG_GETARG_POINTER(0);
	GeoCoord    *b = (GeoCoord *) PG_GETARG_POINTER(1);

	PG_RETURN_BOOL(gcoord_cmp_internal(a, b) <= 0);
}

PG_FUNCTION_INFO_V1(gcoord_eq);

Datum
gcoord_eq(PG_FUNCTION_ARGS)
{
	GeoCoord    *a = (GeoCoord *) PG_GETARG_POINTER(0);
	GeoCoord    *b = (GeoCoord *) PG_GETARG_POINTER(1);

	PG_RETURN_BOOL(gcoord_cmp_internal(a, b) == 0);
}

PG_FUNCTION_INFO_V1(gcoord_ge);

Datum
gcoord_ge(PG_FUNCTION_ARGS)
{
	GeoCoord    *a = (GeoCoord *) PG_GETARG_POINTER(0);
	GeoCoord    *b = (GeoCoord *) PG_GETARG_POINTER(1);

	PG_RETURN_BOOL(gcoord_cmp_internal(a, b) >= 0);
}

PG_FUNCTION_INFO_V1(gcoord_gt);

Datum
gcoord_gt(PG_FUNCTION_ARGS)
{
	GeoCoord    *a = (GeoCoord *) PG_GETARG_POINTER(0);
	GeoCoord    *b = (GeoCoord *) PG_GETARG_POINTER(1);

	PG_RETURN_BOOL(gcoord_cmp_internal(a, b) > 0);
}

PG_FUNCTION_INFO_V1(gcoord_neq);

Datum
gcoord_neq(PG_FUNCTION_ARGS)
{
	GeoCoord    *a = (GeoCoord *) PG_GETARG_POINTER(0);
	GeoCoord    *b = (GeoCoord *) PG_GETARG_POINTER(1);

	PG_RETURN_BOOL(gcoord_cmp_internal(a, b) != 0);
}

PG_FUNCTION_INFO_V1(gcoord_zeq);

Datum
gcoord_zeq(PG_FUNCTION_ARGS)
{
	GeoCoord    *a = (GeoCoord *) PG_GETARG_POINTER(0);
	GeoCoord    *b = (GeoCoord *) PG_GETARG_POINTER(1);

	PG_RETURN_BOOL(gcoord_zone_internal(a, b) > 0);
}

PG_FUNCTION_INFO_V1(gcoord_zneq);

Datum
gcoord_zneq(PG_FUNCTION_ARGS)
{
	GeoCoord    *a = (GeoCoord *) PG_GETARG_POINTER(0);
	GeoCoord    *b = (GeoCoord *) PG_GETARG_POINTER(1);

	PG_RETURN_BOOL(gcoord_zone_internal(a, b) < 0);
}

//support function
PG_FUNCTION_INFO_V1(gcoord_cmp);

Datum
gcoord_cmp(PG_FUNCTION_ARGS)
{
	GeoCoord    *a = (GeoCoord *) PG_GETARG_POINTER(0);
	GeoCoord    *b = (GeoCoord *) PG_GETARG_POINTER(1);

	PG_RETURN_INT32(gcoord_cmp_internal(a, b));
}

PG_FUNCTION_INFO_V1(gcoord_zone_cmp);

Datum
gcoord_zone_cmp(PG_FUNCTION_ARGS)
{
	GeoCoord    *a = (GeoCoord *) PG_GETARG_POINTER(0);
	GeoCoord    *b = (GeoCoord *) PG_GETARG_POINTER(1);

	PG_RETURN_INT32(gcoord_zone_internal(a, b));
}


//hash function

PG_FUNCTION_INFO_V1(gcoord_hash);

Datum
gcoord_hash(PG_FUNCTION_ARGS)
{
	GeoCoord    *gcoord = (GeoCoord *) PG_GETARG_POINTER(0);
	int			hash_code;
	char		*data;
	char		*location_name = get_location(gcoord->data);
	char 		*latitude_dms = convert(get_latitude(gcoord->data));
	char 		latitude_flag = get_latitude_flag(gcoord->data);
	char 		*longitude_dms = convert(get_longitude(gcoord->data));
	char 		longitude_flag = get_longitude_flag(gcoord->data);
	char		*result;

	result = psprintf("%s,%s%c,%s%c", location_name, latitude_dms, latitude_flag, longitude_dms, longitude_flag);
	free(location_name);
	free(latitude_dms);
	free(longitude_dms);

	hash_code = DatumGetUInt32(hash_any((unsigned char *)result, strlen(result)));
	PG_RETURN_INT32(hash_code);
}
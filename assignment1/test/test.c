/*
 * src/tutorial/gcoord.c
 *
 ******************************************************************************
  This file contains routines that can be bound to a Postgres backend and
  called by the backend in the process of processing queries.  The calling
  format for these routines is dictated by Postgres architecture.
******************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
char *convert(float decimal)
{
	int		d, m;
	float	s;
	char 	*dms = (char *) malloc(15);
	char	*s_str = (char *) malloc(7);

	d = (int) decimal;
	sprintf(s_str, "%.4f", decimal - d);
	m = (int)(atoi(s_str+2) * 60 /10000);
	// when convert string to float, there will some small error
	// such as convert '122.42' to float, the result will be 122.419998
	// therefore, s can't be directly converted to int
	s = (int)((3600 * (atoi(s_str+2)) - 60 * m * 10000) / 10000);
	if (m == 0)
		sprintf(dms, "%d°", d);
	else if (s == 0) 
		sprintf(dms, "%d°%d'", d, m);
	else
		sprintf(dms, "%d°%d'%.0f\"", d, m, s);
	free(s_str);
	return dms;
}


int main()
{
	printf("%s\n", convert(122.459998));
}









// bool valid(char *input, char *pattern);

// bool valid(char *input, char *pattern)
// {

// // locationName: [A-Za-z][A-Za-z ]*[A-Za-z]
// // latitude: (([1-8]?[0-9](\\.[0-9]{1,4})?)|(90(\\.0{1,4})?))°[NS]
// // longitude: (([0-9]{1,2}(\\.[0-9]{1,4})?)|(1[0-7][0-9](\\.[0-9]{1,4})?)|180(\\.0{1,4})?)°[WE]

// ([A-Za-z][A-Za-z ]*[A-Za-z],(([1-8]?[0-9](\\.[0-9]{1,4})?)|(90(\\.0{1,4})?))°[NS][, ](([0-9]{1,2}(\\.[0-9]{1,4})?)|(1[0-7][0-9](\\.[0-9]{1,4})?)|180(\\.0{1,4})?)°[WE])|([A-Za-z][A-Za-z ]*[A-Za-z],(([0-9]{1,2}(\\.[0-9]{1,4})?)|(1[0-7][0-9](\\.[0-9]{1,4})?)|180(\\.0{1,4})?)°[WE][, ](([1-8]?[0-9](\\.[0-9]{1,4})?)|(90(\\.0{1,4})?))°[NS])


// 	//char 	*pattern_1 = '^[A-Za-z ]+[A-Za-z],(0?\d{1,2}(\.\d{1,})|1[0-7]?\d{1}(\.\d{1,})?|180(\.0{1,})?)°[WE][, ]([0-8]?\d{1}\.\d{2}|90\.0{2}|[0-8]?\d{1}|90)°[NS]$';
// 	//char	*pattern_2 = '^[A-Za-z ]+[A-Za-z],([0-8]?\d{1}\.\d{2}|90\.0{2}|[0-8]?\d{1}|90)°[NS][, ](0?\d{1,2}(\.\d{1,})|1[0-7]?\d{1}(\.\d{1,})?|180(\.0{1,})?)°[WE]$';

// 	regex_t regex;
// 	bool 	valid_flag = false;
// 	if (regcomp(&regex, pattern, REG_EXTENDED |REG_ICASE |REG_NOSUB) == 0)
// 	{
// 		if (regexec(&regex, input, 0, NULL, 0) == 0)
// 		{
// 			valid_flag = true;
// 		}
// 	}
// 	regfree(&regex);
// 	return valid_flag;
// }

// int main()
// {
// 	char 		*str = "Melbourne,37.84°S,144.95°E"; //copy str for split
// 	char		*str_copy = (char *) malloc(strlen(str) + 1);

// 	// char *str_copy = "san francisco,122.42°W 37.77°N";
// 	// char *str_copy = "Melbourne,-37.84°S,144.95°E";
// 	// char *str_copy = " San Francisco\,37.77°N,122.4194°W";
// 	// char *str_copy = "Melbourne:37.84°S:144.95°E";

// 	strcpy(str_copy, str);

// 	token = strtok(str_copy, ",");
// 	printf("%s\n",token);
// 	// if locationName is invalid, report error
// 	location_name = (char *) malloc(strlen(token) + 1);
// 	strcpy(location_name, token);
// 	// latitude or longitude
// 	token = strtok(NULL, ", ");
// 	latitude_str = (char *) malloc(strlen(token) + 1);
// 	strcpy(latitude_str, token);
// 	token = strtok(NULL, ", ");
// 	longitude_str = (char *) malloc(strlen(token) + 1);
// 	token2 = strtok(latitude_str, "°");
// 	printf("%s\n",token2);
// 	token2 = strtok(NULL, "°");
// 	printf("%s\n",token2);

	
	
// 	strcpy(longitude_str, token);
// 	token2 = strtok(longitude_str, "°");
// 	printf("%s\n",token2);
// 	token2 = strtok(NULL, "°");
// 	printf("%s\n",token2);

// 	// for(int i = 0; i <= 3; i++)
// 	// {
// 	// 		// split with "," or " "
// 	// 		token = strtok(NULL, ", ");
// 	// 		printf("%s\n", token);
// 	// 		token2 = strtok(token, "°"); 
// 	// 		printf("%s\n",token2);
// 	// 		token2 = strtok(NULL, "°");
// 	// 		printf("%s\n",token2);

// 	// }
// 	return 0;
// }


// int main(){
// 	char 		*str_copy = "Melbourne,37.84°S,144.95°E"; //copy str for split
// 	char		*location_name;
// 	float 		latitude;
// 	char		latitude_flag;
// 	float		longitude;
// 	char		longitude_flag;
// 	char 		*token;  //split ',' or ' '
// 	char		*token2; //split '°'
// 	char		*str_location;
// 	// char *str_copy = "san francisco,122.42°W 37.77°N";
// 	// char *str_copy = "Melbourne,-37.84°S,144.95°E";
// 	// char *str_copy = " San Francisco\,37.77°N,122.4194°W";
// 	// char *str_copy = "Melbourne:37.84°S:144.95°E";

// 	// char *input = PG_GETARG_CSTRING(0);
// 	token = strtok(str_copy, ",");
// 	// if locationName is invalid, report error
// 	if (!valid(token,"^[A-Za-z][A-Za-z ]*[A-Za-z]$"))
// 		ereport(ERROR,
// 				(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
// 				 errmsg("invalid input syntax for type %s: \"%s\"",
// 						"GeoCoord", str)));

// 	location_name = (char *) malloc(strlen(token) + 1);
// 	strcpy(location_name, token);
// 	// latitude or longitude
// 	for (int i = 0; i <= 1; i++)
// 	{
// 		if (token != NULL)
// 		{
// 			// split with "," or " "
// 			token = strtok(NULL, ", ");
// 			if (!valid(token, "^(([1-8]?[0-9](\\.[0-9]{1,4})?)|(90(\\.0{1,4})?))°[NS]$"))
// 			{
// 				token2 = strtok(token, "°"); // 对每个token1再次分割
// 				latitude = atof(token);
// 				token2 = strtok(NULL, "°");
// 				latitude_flag = token2[0];
// 			}
// 			else if (!valid(token, "^(([0-9]{1,2}(\\.[0-9]{1,4})?)|(1[0-7][0-9](\\.[0-9]{1,4})?)|180(\\.0{1,4})?)°[WE]$"))
// 			{
// 				token2 = strtok(token, "°"); // 对每个token1再次分割
// 				longitude = atof(token);
// 				token2 = strtok(NULL, "°");
// 				longitude_flag = token2[0];
// 			}
// 			else
// 			{
// 				ereport(ERROR,
// 						(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
// 						errmsg("invalid input syntax for type %s: \"%s\"",
// 								"GeoCoord", str)));
// 			}
// 		}
// 		else
// 		{
// 			ereport(ERROR,
// 					(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
// 					errmsg("invalid input syntax for type %s: \"%s\"",
// 							"GeoCoord", str)));
// 		}
// 	}
// 	printf("%s,%f°%c,%f°%c",location_name,latitude,latitude_flag,longitude,longitude_flag);
// 	return 0;
// }





///////////////////////////判断locationName是否合法输入，并且获取整行
// int main()
// {
// 	char input[61];
// 	printf("input locationName\n");
// 	fgets(input, 61, stdin);
// 	sscanf(input, "%60[^\n]", input);
// 	if (valid(input,"^[A-Za-z][A-Za-z ]*[A-Za-z]$"))
// 	{
// 		printf("success\n");
// 	}
// 	else
// 	{
// 		printf("fail\n");
// 	}
// }




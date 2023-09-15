#include <string.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>

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

int main()
{
	char	   	*str = "Melbourne,144.95°E,37.84°S";
	char		*location_name;
	char		*latitude;
	char		*longitude;
	char 		*token;
	char		*str_copy = (char *) malloc(strlen(str) + 1);

	strcpy(str_copy, str);
	token = strtok(str_copy, ", ");
	location_name = (char *) malloc(strlen(token) + 1);
	strcpy(location_name, token);
	for(int i = 0; location_name[i]; i++)
	{
		location_name[i] = tolower(location_name[i]);
	}

	while(token != NULL)
	{
		if (valid(token, "^(([1-8]?[0-9](\\.[0-9]{1,4})?)|(90(\\.0{1,4})?))°[NS]$"))
		{
			latitude = (char *) malloc(strlen(token) + 1);
			strcpy(latitude, token);
		}

		else if (valid(token, "^(([0-9]{1,2}(\\.[0-9]{1,4})?)|(1[0-7][0-9](\\.[0-9]{1,4})?)|180(\\.0{1,4})?)°[WE]$"))
		{
			longitude = (char *) malloc(strlen(token) + 1);
			strcpy(longitude, token);	
		}

		token = strtok(NULL, ", ");
	}
	printf("%s",location_name);
	printf("%s",latitude);
	printf("%s\n",longitude);
	sprintf(str_copy, "(%s,%s,%s)", location_name, latitude, longitude);
	printf(str_copy);
	free(location_name);
	free(latitude);
	free(longitude);
	free(str_copy);
	return 0;
}

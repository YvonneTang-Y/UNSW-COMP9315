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
	char	   	*str = "Melbourne,37.84°S,144.95°E";
	char 		*token;
	char		*str_copy = (char *) malloc(strlen(str) + 1);
    if (valid(str, "^([A-Za-z][A-Za-z ]*[A-Za-z],(([1-8]?[0-9](\\.[0-9]{1,4})?)|(90(\\.0{1,4})?))°[NS][, ](([0-9]{1,2}(\\.[0-9]{1,4})?)|(1[0-7][0-9](\\.[0-9]{1,4})?)|180(\\.0{1,4})?)°[WE])|([A-Za-z][A-Za-z ]*[A-Za-z],(([0-9]{1,2}(\\.[0-9]{1,4})?)|(1[0-7][0-9](\\.[0-9]{1,4})?)|180(\\.0{1,4})?)°[WE][, ](([1-8]?[0-9](\\.[0-9]{1,4})?)|(90(\\.0{1,4})?))°[NS])$"))
        printf("success");
    else
        printf("fail");
	return 0;
}
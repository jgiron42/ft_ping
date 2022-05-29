#include "ft_ping.h"

/**
 * three way compare lexicographically strings a and b
 * @return -1 if a < b, 0 if a == b, 1 if a > b (lexicographically)
 */
static int	bigint_cmp(char *a, char *b)
{
	int i = 0;
	int j = 0;

	if (*a == '-' && *b == '-')
	{
		a++;
		b++;
	}
	else if (*a == '-' && *b != '-')
		return -1;
	else if (*a != '-' && *b == '-')
		return 1;
	while (*a == '0')
		a++;
	while (*b == '0')
		b++;
	while (ft_isdigit(a[i]))
		i++;
	while (ft_isdigit(b[j]))
		j++;
	if (i < j)
		return -1;
	if (i > j)
		return 1;
	i = 0;
	while (ft_isdigit(a[i]))
	{
		if (a[i] < b[i])
			return -1;
		if (a[i] > b[i])
			return 1;
		i++;
	}
	return (0);
}

/**
 * check if the string str s a valid serialized integer
 * @param str the string
 * @param is_signed whether or not the int can begin with a +/-
 * @param terminator the expected character after the integer
 * @return true if the string is a valid integer
 */
bool int_validator(char *str, bool is_signed, char terminator)
{
	while(ft_isspace(*str))
		str++;
	if ((*str == '-' || *str == '+') && is_signed)
		str++;
	if (!ft_isdigit(*str))
		return (false);
	int i = 0;
	while (ft_isdigit(str[i]))
		i++;
	return (str[i] == terminator);
}

/**
 * check whether the serialized integer pointed to by str is strictly in the range between lower_bound and upper_bound
 * @return true if the integer is between lower_bound and upper_bound
 */
bool	range_validator(char *str, char *lower_bound, char *upper_bound)
{
	while(!ft_isdigit(*str) && *str != '-')
		str++;
	return (bigint_cmp(str, upper_bound) == -1 && bigint_cmp(str, lower_bound) == 1);
}

/**
 * check whether the string str designate a valid ipv4 address
 * @return true if str designate a valid ipv4 address
 */
bool	ipv4_validator(char *str)
{
	for (int i = 0; i < 4; i++)
	{
		if (	!int_validator(str, false, i == 3 ? 0 : '.')
			||	!range_validator(str, "-1", "256"))
			return (false);
		while(ft_isdigit(*str))
			str++;
		if (i == 3)
			break;
		if (*str != '.')
			return (false);
		str++;
	}
	return (true);
}

bool	is_broadcast(char *ip)
{
	ip += ft_strlen(ip) - 3;
	return (!ft_strcmp(ip, "255"));
}
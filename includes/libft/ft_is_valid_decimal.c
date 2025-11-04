/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_is_valid_decimal.c                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gfontao- <gfontao-@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/21 02:15:28 by gfontao-          #+#    #+#             */
/*   Updated: 2025/10/21 02:17:07 by gfontao-         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "libft.h"

bool	ft_is_valid_decimal(const char *str)
{
	int		i;
	bool	has_dot;

	i = 0;
	has_dot = false;
	if (!str || str[0] == '\0')
		return (false);
	while (str[i])
	{
		if (str[i] == '.')
		{
			if (has_dot)
				return (false);
			has_dot = true;
		}
		else if (str[i] < '0' || str[i] > '9')
			return (false);
		i++;
	}
	if (i == 0 || (i == 1 && str[0] == '.'))
		return (false);
	return (true);
}

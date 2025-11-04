/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_strjoin.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gfontao- <gfontao-@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/09/27 14:02:11 by gfontao-          #+#    #+#             */
/*   Updated: 2024/02/27 18:09:16 by gfontao-         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "libft.h"

char	*ft_strjoin(char const *s1, char const *s2)
{
	size_t	ctd;
	size_t	ctd2;
	char	*str;

	ctd = 0;
	str = (char *)malloc(ft_strlen(s1) + ft_strlen(s2) + 1);
	if (!str)
		return (NULL);
	while (s1 && s1[ctd])
	{
		str[ctd] = s1[ctd];
		ctd++;
	}
	ctd2 = 0;
	while (s2 && s2[ctd2])
	{
		str[ctd] = s2[ctd2];
		ctd++;
		ctd2++;
	}
	str[ctd] = '\0';
	return (str);
}

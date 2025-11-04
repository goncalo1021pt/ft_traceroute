/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_strcmp.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gfontao- <gfontao-@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/21 01:51:12 by gfontao-          #+#    #+#             */
/*   Updated: 2025/10/21 01:56:20 by gfontao-         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "libft.h"

int	ft_strcmp(const char *s1, const char *s2)
{
	size_t	ctd;

	ctd = 0;
	while (s1[ctd] && s2[ctd] && s1[ctd] == s2[ctd])
		ctd++;
	return ((unsigned char)s1[ctd] - (unsigned char)s2[ctd]);
}

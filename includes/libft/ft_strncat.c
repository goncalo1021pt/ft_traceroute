/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_strncat.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gfontao- <gfontao-@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/21 01:51:42 by gfontao-          #+#    #+#             */
/*   Updated: 2025/10/21 01:51:42 by gfontao-         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "libft.h"

char	*ft_strncat(char *dest, const char *src, size_t n)
{
	char	*od;
	size_t	ctd;

	ctd = 0;
	od = dest;
	dest += ft_strlen(dest);
	while (ctd < ft_strlen(src) && ctd < n)
	{
		*dest = src[ctd];
		dest++;
		ctd++;
	}
	*dest = '\0';
	return (od);
}

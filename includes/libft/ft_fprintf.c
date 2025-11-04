/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_fprintf.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gfontao- <gfontao-@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/21 01:48:36 by gfontao-          #+#    #+#             */
/*   Updated: 2025/10/21 01:49:03 by gfontao-         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "libft.h"

int	ptraux_fd(int fd, unsigned long int ptr)
{
	int	len;

	len = 0;
	if (!ptr)
		len += ft_putstr_fd("(nil)", fd);
	else
	{
		len += write(fd, "0x", 2);
		len += ft_putpt_fd(fd, ptr, EX_LOW_BASE, 0);
	}
	return (len);
}

int	straux_fd(int fd, char *str)
{
	if (!str)
		return (ft_putstr_fd("(null)", fd));
	else
		return (ft_putstr_fd(str, fd));
}

int	flag_type_fd(int fd, char c, va_list *args)
{
	int	len;

	len = 0;
	if (c == 'c')
		len += ft_putchar_fd((va_arg(*args, int)), fd);
	if (c == 's')
		len += straux_fd(fd, va_arg(*args, char *));
	if (c == 'p')
	{
		len += ptraux_fd(fd, va_arg(*args, long int));
	}
	if (c == 'd' || c == 'i')
		len += ft_putnbrt_fd(fd, va_arg(*args, int), 0);
	if (c == 'u')
		len += ft_putunbr_fd(fd, va_arg(*args, unsigned int), 0);
	if (c == 'x')
		len += ft_putnbr_base_fd(fd, va_arg(*args, long int), EX_LOW_BASE, 0);
	if (c == 'X')
		len += ft_putnbr_base_fd(fd, va_arg(*args, long int), EX_UP_BASE, 0);
	if (c == '%')
		len += write(fd, "%", 1);
	return (len);
}

static int	is_char(const char *str, const char c)
{
	int	ctd;

	ctd = 0;
	while (str[ctd])
	{
		if (str[ctd] == c)
			return (1);
		ctd++;
	}
	return (0);
}

int	ft_fprintf(int fd, const char *str, ...)
{
	int		len;
	int		ctd;
	va_list	args;

	if (!str)
		return (-1);
	ctd = 0;
	len = 0;
	va_start(args, str);
	while (str[ctd])
	{
		if (str[ctd] == '%' && is_char(PRINTF_FLAGS, str[ctd + 1]))
		{
			ctd++;
			len += flag_type_fd(fd, str[ctd], &args);
		}
		else
			len += write(fd, &str[ctd], 1);
		ctd++;
	}
	va_end(args);
	return (len);
}

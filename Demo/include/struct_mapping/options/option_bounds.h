#pragma once

#include "../utility.h"
#include "../exception.h"

#include <limits>
#include <string>

namespace struct_mapping
{

template<typename T>
class Bounds
{
public:
	Bounds(T lower_, T upper_)
		:	lower(lower_), upper(upper_) {}

	template<typename M>
	void check_option(const std::string& name) const
	{
		static_assert(
			detail::is_integer_or_floating_point_v<detail::remove_optional_t<M>>,
			"bad option (Bounds): option can only be applied to types: integer or floating point");

		static_assert(
			!detail::is_integer_v<detail::remove_optional_t<M>> || detail::is_integer_v<T>,
			"bad option (Bounds): type error, expected integer");

		static_assert(
			!std::is_floating_point_v<detail::remove_optional_t<M>> || detail::is_integer_or_floating_point_v<T>,
			"bad option (Bounds): type error, expected integer or floating point");

		if constexpr (detail::is_integer_or_floating_point_v<detail::remove_optional_t<M>>)
		{
			if (!in_limits<detail::remove_optional_t<M>>(lower))
			{
				throw StructMappingException(
					"bad option (Bounds) for '"
						+ name
						+ "': lower = "
						+ std::to_string(lower)
						+ " is out of limits of type ["
						+	std::to_string(std::numeric_limits<detail::remove_optional_t<M>>::lowest())
						+	" : "
						+	std::to_string(std::numeric_limits<detail::remove_optional_t<M>>::max()) + "]");
			}

			if (!in_limits<detail::remove_optional_t<M>>(upper))
			{
				throw StructMappingException(
					"bad option (Bounds) for '"
						+ name
						+ "': upper = "
						+ std::to_string(upper)
						+ " is out of limits of type ["
						+	std::to_string(std::numeric_limits<detail::remove_optional_t<M>>::lowest())
						+	" : "
						+	std::to_string(std::numeric_limits<detail::remove_optional_t<M>>::max()) + "]");
			}

			if (lower > upper)
			{
				throw StructMappingException(
					"bad option (Bounds) for '"
						+ name
						+ "': upper = "
						+ std::to_string(upper)
						+ " less then lower = "
						+ std::to_string(lower));
			}
		}
	}

	template<typename V>
	void operator()(V value, const std::string& name) const
	{
		detail::remove_optional_t<V> value_;

		if constexpr (detail::is_optional_v<V>)
		{
			value_ = value.value();
		}
		else
		{
			value_ = value;
		}

		if constexpr (detail::is_integer_v<T>)
		{
			if (static_cast<long long>(value_) < static_cast<long long>(lower)
					|| static_cast<long long>(value_) > static_cast<long long>(upper))
			{
				throw StructMappingException(
					"value "
						+ std::to_string(value_)
						+ " for '"
						+ name
						+ "' is out of bounds ["
						+ std::to_string(lower)
						+ ", "
						+ std::to_string(upper)
						+ "]");
			}
		}
		else if constexpr (std::is_floating_point_v<T>)
		{
			if (static_cast<double>(value_) < static_cast<double>(lower)
				|| static_cast<double>(value_) > static_cast<double>(upper))
			{
				throw StructMappingException(
					"value "
					+ std::to_string(value_)
					+ " for '"
					+ name
					+ "' is out of bounds ["
					+ std::to_string(lower)
					+ ", "
					+ std::to_string(upper)
					+ "]");
			}
		}
	}

private:
	template<typename M>
	bool in_limits(T value) const
	{
		if constexpr (detail::is_integer_v<M>)
		{
			return static_cast<long long>(value) >= static_cast<long long>(std::numeric_limits<M>::lowest())
				&& static_cast<long long>(value) <= static_cast<long long>(std::numeric_limits<M>::max());
		}
		else
		{
			return static_cast<double>(value) >= static_cast<double>(std::numeric_limits<M>::lowest())
				&& static_cast<double>(value) <= static_cast<double>(std::numeric_limits<M>::max());
		}

		return true;
	}

private:
	T lower;
	T upper;
};

} // struct_mapping
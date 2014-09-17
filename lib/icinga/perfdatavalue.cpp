/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2014 Icinga Development Team (http://www.icinga.org)    *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License                *
 * as published by the Free Software Foundation; either version 2             *
 * of the License, or (at your option) any later version.                     *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program; if not, write to the Free Software Foundation     *
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.             *
 ******************************************************************************/

#include "icinga/perfdatavalue.hpp"
#include "base/convert.hpp"
#include "base/exception.hpp"
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

using namespace icinga;

REGISTER_TYPE(PerfdataValue);

PerfdataValue::PerfdataValue(void)
{ }

PerfdataValue::PerfdataValue(String label, double value, bool counter,
    const String& unit, const Value& warn, const Value& crit, const Value& min,
    const Value& max)
{
	SetLabel(label);
	SetValue(value);
	SetCounter(counter);
	SetUnit(unit);
	SetWarn(warn);
	SetCrit(crit);
	SetMin(min);
	SetMax(max);
}

PerfdataValue::Ptr PerfdataValue::Parse(const String& perfdata)
{
	size_t eqp = perfdata.FindFirstOf('=');

	if (eqp == String::NPos)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid performance data value: " + perfdata));

	String label = perfdata.SubStr(0, eqp);

	if (label.GetLength() > 2 && label[0] == '\'' && label[label.GetLength() - 1] == '\'')
		label = label.SubStr(1, label.GetLength() - 2);

	size_t spq = perfdata.FindFirstOf(' ', eqp);

	if (spq == String::NPos)
		spq = perfdata.GetLength();

	String valueStr = perfdata.SubStr(eqp + 1, spq - eqp - 1);
		
	size_t pos = valueStr.FindFirstNotOf("+-0123456789.e");

	double value = Convert::ToDouble(valueStr.SubStr(0, pos));

	std::vector<String> tokens;
	boost::algorithm::split(tokens, valueStr, boost::is_any_of(";"));

	bool counter = false;
	String unit;
	Value warn, crit, min, max;

	if (pos != String::NPos)
		unit = valueStr.SubStr(pos, tokens[0].GetLength() - pos);

	boost::algorithm::to_lower(unit);

	double base = 1.0;

	if (unit == "us") {
		base /= 1000.0 * 1000.0;
		unit = "seconds";
	} else if (unit == "ms") {
		base /= 1000.0;
		unit = "seconds";
	} else if (unit == "s") {
		unit = "seconds";
	} else if (unit == "tb") {
		base *= 1024.0 * 1024.0 * 1024.0 * 1024.0;
		unit = "bytes";
	} else if (unit == "gb") {
		base *= 1024.0 * 1024.0 * 1024.0;
		unit = "bytes";
	} else if (unit == "mb") {
		base *= 1024.0 * 1024.0;
		unit = "bytes";
	} else if (unit == "kb") {
		base *= 1024.0;
		unit = "bytes";
	} else if (unit == "b") {
		unit = "bytes";
	} else if (unit == "%") {
		unit = "percent";
	} else if (unit == "c") {
		counter = true;
		unit = "";
	} else if (unit != "") {
		BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid performance data unit: " + unit));
	}

	if (tokens.size() > 1 && tokens[1] != "U" && tokens[1] != "")
		warn = Convert::ToDouble(tokens[1]);

	if (tokens.size() > 2 && tokens[2] != "U" && tokens[2] != "")
		crit = Convert::ToDouble(tokens[2]);

	if (tokens.size() > 3 && tokens[3] != "U" && tokens[3] != "")
		min = Convert::ToDouble(tokens[3]);

	if (tokens.size() > 4 && tokens[4] != "U" && tokens[4] != "")
		max = Convert::ToDouble(tokens[4]);

	value = value * base;

	if (!warn.IsEmpty())
		warn = warn * base;

	if (!crit.IsEmpty())
		crit = crit * base;

	if (!min.IsEmpty())
		min = min * base;

	if (!max.IsEmpty())
		max = max * base;

	return make_shared<PerfdataValue>(label, value, counter, unit, warn, crit, min, max);
}

String PerfdataValue::Format(void) const
{
	std::ostringstream result;

	if (GetLabel().FindFirstOf(" ") != String::NPos)
		result << "'" << GetLabel() << "'";
	else
		result << GetLabel();

	result << "=" << Convert::ToString(GetValue());

	String unit;

	if (GetCounter())
		unit = "c";
	else if (GetUnit() == "seconds")
		unit = "s";
	else if (GetUnit() == "percent")
		unit = "%";
	else if (GetUnit() == "bytes")
		unit = "B";

	result << unit;

	if (!GetWarn().IsEmpty()) {
		result << ";" << Convert::ToString(GetWarn());

		if (!GetCrit().IsEmpty()) {
			result << ";" << Convert::ToString(GetCrit());

			if (!GetMin().IsEmpty()) {
				result << ";" << Convert::ToString(GetMin());

				if (!GetMax().IsEmpty()) {
					result << ";" << Convert::ToString(GetMax());
				}
			}
		}
	}

	return result.str();
}

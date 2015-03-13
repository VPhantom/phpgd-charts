<?php

// Class: Studies
//
// Statistical studies for OHLC stock charts.
class Studies {

	// Section: Introduction

	// Topic: Legal
	//
	// Copyright (C) 2008 Stephane Lavergne <http://www.imars.com/>
	//
	// This program is free software: you can redistribute it and/or modify it
	// under the terms of the GNU General Public License as published by the
	// Free Software Foundation, either version 3 of the License, or (at your
	// option) any later version.
	//
	// This program is distributed in the hope that it will be useful, but
	// *WITHOUT ANY WARRANTY*; without even the implied warranty of
	// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
	// Public License for more details.
	//
	// You should have received a copy of the GNU General Public License along
	// with this program. If not, see <http://www.gnu.org/licenses/>.

	// Topic: Description
	// 
	// Unique PHP implementation of stock chart studies. I couldn't find any
	// decent stock charting implementations in PHP, so <ChartOHLC> and these
	// studies were born. This class is merely a namespace for static
	// functions, each operating on the same "TOHLCV" array used to initialize
	// <ChartOHLC>.

	// Group: TOHLCV Functions

	// Function: MA
	//
	// Simple moving average.
	//
	// Parameters:
	// data - "TOHLCV" array, same as needed by <ChartOHLC>.
	// N    - How many bars wide to consider.
	//
	// Returns:
	// Indexed array where the first _N_ values are NULL to match the _data_
	// array offsets.
	function MA($data, $N) {
		$result = Array();
		$len = count($data);
		$dd = Array();
		if ($len > $N) {
			for ($i=0; $i < $N - 1; $i++) {
				$result[] = null;
				array_push($dd, $data[$i][4]);
			};
			for ( ; $i < $len; $i++) {
				array_push($dd, $data[$i][4]);
				$result[] = array_sum($dd) / count($dd);
				array_shift($dd);
			};
		};
		return $result;
	}

	// Function: DonchianChannel
	//
	// Donchian channel.
	//
	// Parameters:
	// data - "TOHLCV" array, same as needed by <ChartOHLC>.
	// N    - How many bars wide to consider.
	//
	// Returns:
	// Indexed array of 2 indexed arrays containing, respectively, the upper
	// and lower channel boundary. The first _N_ values of each array are NULL
	// to match the _data_ array offsets.
	function DonchianChannel($data, $N) {
		$result = Array(Array(), Array());
		$len = count($data);
		$dh = Array();
		$dl = Array();
		if ($len > $N) {
			for ($i=0; $i < $N - 1; $i++) {
				$result[0][] = null;
				$result[1][] = null;
				array_push($dh, $data[$i][2]);
				array_push($dl, $data[$i][3]);
			};
			for ( ; $i < $len; $i++) {
				array_push($dh, $data[$i][2]);
				array_push($dl, $data[$i][3]);
				$max = $dh[0];
				$min = $dl[0];
				for ($j=1; $j < count($dh); $j++) {
					$max = ($dh[$j] > $max) ? $dh[$j] : $max;
					$min = ($dl[$j] < $min) ? $dl[$j] : $min;
				};
				$result[0][] = $max;
				$result[1][] = $min;
				array_shift($dh);
				array_shift($dl);
			};
		};
		return $result;
	}

	// Function: BollingerBands
	//
	// Compute Bollinger Bands from price data.
	//
	// Parameters:
	// data      - "TOHLCV" array, same as needed by <ChartOHLC>.
	// N         - How many bars wide to consider.
	// K         - How many standard deviations above and below average to
	//             return.
	//
	// Returns:
	// An indexed array of 3 indexed arrays containing, respectively, average +
	// _K_ standard deviations, the average, and average - _K_ standard
	// deviations. The first _N_ values of each array are NULL to match the
	// _data_ array offsets.
	static function BollingerBands($data, $N, $K) {
		// FIXME: Did not confirm that the initial array has exactly $N
		// elements, and always thereafter. Are we good?
		$result = Array(Array(), Array(), Array());
		$dd = Array();
		$len = count($data);
		if ($len > $N) {
			for ($i=0; $i < $N - 1; $i++) {
				$result[0][] = null;
				$result[1][] = null;
				$result[2][] = null;
				array_push($dd, $data[$i][4]);
			};
			for ( ; $i < $len; $i++) {
				array_push($dd, $data[$i][4]);
				$dev = Studies::stdDev($dd);
				$avg = array_sum($dd) / count($dd);
				$result[0][] = $avg + ($K * $dev);
				$result[1][] = $avg;
				$result[2][] = $avg - ($K * $dev);
				array_shift($dd);
			};
		}
		return $result;
	}

	// Function: Pivots
	//
	// Compute pivots from price data (experimental.)
	//
	// Parameters:
	// data       - "TOHLCV" array, same as needed by <ChartOHLC>.
	// threshold  - Minimum advantage-over-peers to report. (Minimum 0, should
	//              typically be set below 30 to get results.)
	// low, high  - Price range to consider.
	// increment  - Price increment between levels to test. (A small value is
	//              recommended. Perhaps 0.10 or less for stocks.)
	//
	// Returns:
	// An indexed array of price-advantage pairs. Only prices meeting the
	// _threshold_ are included in this result. The advantage returned is
	// therefore at least 0, and does not exceed a theoretical 100. In practice
	// however, results over 30 are extremely unlikely.
	static function Pivots($data, $threshold, $low, $high, $increment) {
		$result = Array();

/*

A chaque barre, on considere qu'on traverse tout de low a high.
On calcule ensuite la somme des deux plus longs pour chaque price point.
Enfin, on calcule l'avantage de chaque price point sur ses voisins en ajoutant au
resultat ceux qui atteignent threshold.

*/

		$low = floor(($low - $increment) / $increment) * $increment;
		$prices = Array();
		for ($i = $low; $i <= $high; $i += $increment) {
			$prices[] = Array($i, Array(), 0, 0, 0, 0);
		};

		$bars = count($data);
		for ($i = 0; $i < $bars; $i++) {
			for ($j = ceil(($data[$i][3] - $low) / $increment); $prices[$j][0] < $data[$i][2]; $j++) {
				$prices[$j][1][] = $i;
			};
		};

		foreach ($prices as $i => $price) {
			$lastSeen = 0;
			$prices[$i][1][] = $bars;

/*
			// Algorithm 1:
			//
			// Keep the longest 2 widths for a given price level.
			// Score: their sum divided by the period width, as a percentage.
			//
			// Variant 2: Never use widths < 25% of period width.
			// Variant 3: Never use widths < 50% of period width.
			// Variant 4: Use longest 3 widths, not 2.
			// 
			foreach ($prices[$i][1] as $seen) {
				$width = $seen - $lastSeen;
				$lastSeen = $seen;
				// If bigger than 20%, try saving
				// if (($width / 0.20) >= $bars) {
					if ($width > $prices[$i][2]) {
						$prices[$i][4] = $prices[$i][3];
						$prices[$i][3] = $prices[$i][2];
						$prices[$i][2] = $width;
					} elseif ($width > $prices[$i][3]) {
						$prices[$i][4] = $prices[$i][3];
						$prices[$i][3] = $width;
					} elseif ($width > $prices[$i][4]) {
						$prices[$i][4] = $width;
					};
					// Else discard, we still have the 2 widest.
				};
			// };
			$prices[$i][5] = ($prices[$i][2] + $prices[$i][3] + $prices[$i][4]) * 100 / $bars;
*/

			// Algorithm 2:
			//
			// Square root of the average of ALL squared widths.
			//
			$score = 0;
			$scoreCount = 0;
			foreach ($prices[$i][1] as $seen) {
				$score += ($seen - $lastSeen) * ($seen - $lastSeen);
				$lastSeen = $seen;
				$scoreCount++;
			};
			// Variant SQRT 1:
			// $prices[$i][5] = (sqrt($score) / $scoreCount) * 100 / $bars;
			// Variant SQRT 2:
			$prices[$i][5] = (sqrt($score / $scoreCount)) * 100 / $bars;


		};

		$maxPrices = count($prices) - 1;
		$lastScore = 100;
		for ($i = 0; $i < $maxPrices; $i++) {
			$delta = ($prices[$i][5] - $lastScore) + ($prices[$i][5] - $prices[$i+1][5]);
			$lastScore = $prices[$i][5];
			if ($delta >= $threshold) $result[] = Array($prices[$i][0], $delta);
		};

		return $result;
	}

	// Group: Statistical Functions

	// Function: stdDev
	//
	// Standard deviation of a series.
	//
	// Parameters:
	// a - Indexed array of values.
	//
	// Returns:
	// Standard deviation.
	static function stdDev($a) {
		$result = 0.0;
		$variance = 0.0;
		$count = count($a);
		$mean = array_sum($a) / $count;
		foreach ($a as $i) {
			$variance += ($i - $mean) * ($i - $mean);
		};
		$result = pow($variance / $count, 0.5);
		return $result;
	}

}

?>

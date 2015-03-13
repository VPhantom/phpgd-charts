<?php

// Class: ChartOHLC
//
// Plot OHLC stock charts.
class ChartOHLC {

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
	// Unique PHP implementation of stock charting. I couldn't find any *bar*
	// chart implementations in PHP, so ChartOHLC was born. See
	// <http://www.imars.com/trading/> for real-life examples of charts made
	// with this class.

	// Topic: Usage
	//
	// Couldn't be simpler, for my needs anyway. :-)
	//
	// (begin example)
	// // Let's define a nice clean colour scheme first:
	// $bgColor = 0xFFFFFF;
	// $gridColor = 0xE0E0E0;
	// $titleColor = 0x009000;
	// $upColor = $titleColor;
	// $downColor = false;
	// $borderColor = 0xFF8080;
	// $labelColor = 0x000000;
	// $volColor = 0x8080FF;
	//
	// // Now, build the chart where $data is our array:
	// require_once 'ChartOHLC.inc';
	// $chart = new ChartOHLC(600, 300, $bgColor, $data);
	// $chart->title("MSFT - 5 min", $titleColor);
	// $chart->title("(C) 2008 imars.com but freely redistributable", $gridColor, true);
	// $chart->plotVolume($volColor, $borderColor, $labelColor, 50);
	// $chart->plotPriceLegends($borderColor, $labelColor, $gridColor, true);
	// $chart->plotPrice($upColor, $downColor);
	//
	// // Dump to stdout and destroy:
	// header('Content-Type: image/png');
	// $chart->png();
	// unset($chart);
	// (end example)

	// Topic: TODO
	//
	// The code has not been optimized at all yet, not even cleaned up. It just
	// does the job for now and I'll clean it up later on.

	// Undocumented properties
	private $x;
	private $y;
	private $img;
	private $colors;
	private $bgColor;
	private $data;
	private $dataLength;
	private $high;
	private $low;
	private $close;
	private $decimals;
	private $chartOriginX;
	private $chartOriginY;
	private $chartX;
	private $chartY;
	private $barWidth;
	private $titleWidth;

	// Group: Constants

	// Constant: LEGEND_HEIGHT
	// Minimum distance in pixels between vertical legend labels.
	const LEGEND_HEIGHT = 25;

	// Group: General Methods

	// Constructor: ChartOHLC
	//
	// Create a new OHLC chart.
	//
	// Parameters:
	// x          - Width in pixels
	// y          - Height in pixels
	// bgColor    - Integer value for the background color. (i.e. "0xFFFFFF")
	// data       - Indexed array of bar information. Each element is an
	//              indexed array listing, in order: timestamp integer, open,
	//              high, low, close price floats, and volume integer.
	// wantPrice  - Bool whether you intend to plot a price legend. (Optional.)
	// wantVolume - Bool whether you intend to plot volume (mandatory legend.)
	//              (Optional.)
	// decimals   - Number of decimals to standardize to in price displays.
	//              (Optional.)
	function ChartOHLC($x, $y, $bgColor, $data, $wantPrice = true, $wantVolume = true, $decimals = 2) {
		$this->x = $x;
		$this->y = $y;
		$this->img = imagecreate($x, $y);
		$this->colors = Array();
		$this->bgColor = $bgColor;
		imagefill($this->img, 0, 0, $this->color($bgColor));
		$this->data = &$data;
		$this->dataLength = count($data);
		$this->chartOriginX = 0;
		$this->chartOriginY = 0;
		$this->chartX = $x;
		$this->chartY = $y;
		$this->close = $data[$this->dataLength - 1][4];
		$this->decimals = $decimals;
		$this->titleWidth = 0;
		$this->setBarWidth(4);
		// Reduce chart area to make room for legend.
		if ($wantPrice || $wantVolume) $this->chartX -= max(
			(strlen(number_format($this->high, $this->decimals, '.', ',')) * imagefontwidth(2)) + 6,
			(strlen(number_format($this->maxVol, 0, '.', ',')) * imagefontwidth(2)) + 4
		);
		// Chicken or the egg? We needed our high anx maxVol to find the legend
		// width, but then to calculate the final, viewable range we need
		// chartX. The least ugly solution is to call setBarWidth() before and
		// after reducing chartX.
		$this->setBarWidth(4);

	}

	// Destructor: __destruct
	//
	// Destroy this instance.
	function __destruct() {
		imagedestroy($this->img);
	}

	// Method: setBarWidth
	//
	// Change the density of the chart. By default, each bar is 4 pixels wide:
	// 1 pixel for the open dot, 1 pixel for the bar itself, 1 pixel for the
	// close dot, and 1 spacer.
	//
	// I personally prefer 3-6 depending on the purpose of the chart. 2-3 looks
	// very cramped, although it's cheaper than buying additional monitors. :-)
	//
	// Careful:
	// If you're changing the bar width with this method, be sure to call it
	// *before* invoking any of the plotting methods.
	//
	// Parameters:
	// width - New width in pixels.
	//
	// Returns:
	// The _width_ parameter.
	function setBarWidth($width) {
		// Gotta recalculate the high/low/maxVol/range for the viewable area only!
		$bars = min(ceil($this->chartX / $width),$this->dataLength);
		$last = $this->dataLength - 1;
		$this->high = $this->data[$last][2];
		$this->low = $this->data[$last][3];
		$this->maxVol = $this->data[$last][5];
		for ($j=1, $i=$last; $j <= $bars; $j++, $i--) {
			$this->high = max($this->high, $this->data[$i][2]);
			$this->low = min($this->low, $this->data[$i][3]);
			$this->maxVol = max($this->maxVol, $this->data[$i][5]);
		};
		$this->range = $this->high - $this->low;
		return $this->barWidth = $width;
	}

	// Method: getLow
	//
	// Get lowest price in the entire data set.
	//
	// If called after <setBarWidth()>, it will be the viewable area *only*.
	//
	// If called after <plotPriceLegends()>, it will be for the viewable area
	// only, but subject to the extending done to reach round numbers on the
	// legend.
	//
	// Returns:
	// The lowest price.
	function getLow() {
		return $this->low;
	}

	// Method: getHigh
	//
	// Get highest price in the entire data set.
	//
	// If called after <setBarWidth()>, it will be the viewable area *only*.
	//
	// If called after <plotPriceLegends()>, it will be for the viewable area
	// only, but subject to the extending done to reach round numbers on the
	// legend.
	//
	// Returns:
	// The highest price.
	function getHigh() {
		return $this->high;
	}

	// Method: png
	//
	// Generate PNG image. The result is either returned in a string or saved
	// to a file, depending on the presence of the _$filename_ argument.
	//
	// Parameters:
	// filename - File to save PNG image to. (Optional.)
	//
	// Returns:
	// The direct result of GD's imagepng(), which should be the PNG data in a
	// string if _$filename_ was omitted.
	function png($filename = null) {
		return imagepng($this->img, $filename, 9);
	}

	// Group: Plotting Methods

	// Method: title
	// 
	// Add a title at the top of the chart. The first time it is invoked, it
	// reduces the height of the OHLC chart area in order to fit a line of text
	// at the top. In subsequent invocations, additional titles are appended at
	// the end of the existing title.
	//
	// Caveats:
	//
	// While multiple left-justified titles can be appended, only one
	// right-justified title can be printed.
	// 
	// Parameters:
	// title - Text to display.
	// color - 24-bit colour integer to print with. (i.e. 0x00FF00 for green)
	// right - Bool whether you want the title right-justified. (Optional.)
	//
	// Returns:
	// No return value.
	function title($title, $color, $right = false) {
		// Reduce chart area to make room for title if this is the first.
		if ($this->titleWidth == 0) {
			$this->chartOriginY = 13;
			$this->chartY -= 13;
		};
		// Draw title
		$newWidth = imagefontwidth(2) * strlen($title) + 3;
		if ($right) {
			imagestring($this->img, 2, $this->chartOriginX + $this->chartX - $newWidth, 0, $title, $this->color($color));
		} else {
			imagestring($this->img, 2, $this->titleWidth + 3, 0, $title, $this->color($color));
			$this->titleWidth += $newWidth;
		};
	}

	// Method: plotPriceLegends
	//
	// Display legend labels and background grid. To stack your prices on top
	// of the grid, be sure to invoke this *before* <plotPrice()>.
	//
	// Parameters:
	// borderColor - 24-bit colour integer for the border
	// priceColor  - 24-bit colour integer for the price
	// gridColor   - 24-bit colour integer for the grid or _false_ if
	//               undesired. (Optional.)
	// doTime      - Bool whether to also process the X (time) axis.
	//               (Optional.)
	// mGridColor  - 24-bit colour integer for the major grid lines or _false_
	//               if undesired. (Optional.)
	//
	// Returns:
	// No return value.
	function plotPriceLegends($borderColor, $priceColor, $gridColor = false, $doTime = true, $mGridColor = false) {
		// CAUTION: Modifies $this->high, $this->low, $this->range to improve legibility.

		if ($doTime) $this->chartY -= 26;
		// Draw legend numbers
		$steps = max(floor($this->chartY / ChartOHLC::LEGEND_HEIGHT), 2);
		$stepPrice = (($this->high - $this->low) / $steps);
		// Lock $stepPrice to closest round interval.
		// This is a quick hack tailored to my preferences. There may be a way
		// to make this universal with a one-liner (via a string in exponential
		// notation perhaps?)
		// FOREX intervals:
		if     ($stepPrice < 0.0005) $stepPrice = 0.0005;
		elseif ($stepPrice < 0.0010) $stepPrice = 0.0010;
		elseif ($stepPrice < 0.0025) $stepPrice = 0.0025;
		elseif ($stepPrice < 0.0050) $stepPrice = 0.0050;
		elseif ($stepPrice < 0.0100) $stepPrice = 0.0100;
		// Stock intervals:
		elseif ($stepPrice < 0.05) $stepPrice = 0.05;
		elseif ($stepPrice < 0.10) $stepPrice = 0.1;
		elseif ($stepPrice < 0.25) $stepPrice = 0.25;
		elseif ($stepPrice < 0.50) $stepPrice = 0.5;
		elseif ($stepPrice < 1) $stepPrice = 1;
		elseif ($stepPrice < 2) $stepPrice = 2;
		elseif ($stepPrice < 5) $stepPrice = 5;
		// Stock/Index intervals:
		elseif ($stepPrice < 10) $stepPrice = 10;
		elseif ($stepPrice < 25) $stepPrice = 25;
		elseif ($stepPrice < 50) $stepPrice = 50;
		elseif ($stepPrice < 100) $stepPrice = 100;
		elseif ($stepPrice < 250) $stepPrice = 250;
		elseif ($stepPrice < 500) $stepPrice = 500;
		elseif ($stepPrice < 1000) $stepPrice = 1000;
		$this->low = floor($this->low / $stepPrice) * $stepPrice;
		$this->high = ceil($this->high / $stepPrice) * $stepPrice;
		$this->range = $this->high - $this->low;
		$steps = floor(($this->high - $this->low) / $stepPrice);

		$gridCol = ($gridColor !== false) ? $this->color($gridColor) : false;
		$mGridCol = ($mGridColor !== false) ? $this->color($mGridColor) : $gridCol;
		$fontVOffset = round(imagefontheight(2) / 2);
		for ($i=0, $j=$steps; $j>0; $i++, $j--) {
			$level = $this->low + ($j * $stepPrice);
			$label = number_format($level, $this->decimals, '.', ',');
			$y = $this->priceY($level);
			imagestring($this->img, 2, $this->chartOriginX + $this->chartX + 5, $y - $fontVOffset, "{$label}", $this->color($priceColor));
			imageline($this->img, $this->chartOriginX + $this->chartX, $y, $this->chartOriginX + $this->chartX + 3, $y, $this->color($borderColor));
			if ($gridCol !== false) {
				imageline($this->img, $this->chartOriginX, $y, $this->chartOriginX + $this->chartX, $y, $gridCol);
			};
		};
		if ($doTime) {
			// We have room for bottom line label.
			$label = number_format($this->low, $this->decimals, '.', ',');
			$x = $this->chartOriginX + $this->chartX;
			$y = $this->chartOriginY + $this->chartY;
			imagestring($this->img, 2, $x + 5, $y - $fontVOffset, "{$label}", $this->color($priceColor));
			imageline($this->img, $x, $y, $x + 3, $y, $this->color($borderColor)); 
		};

		// Draw border
		imageline($this->img, $this->chartX, $this->chartOriginY - $fontVOffset, $this->chartX, $this->chartOriginY + $this->chartY, $this->color($borderColor));

		// Take care of time axis now...
		if ($doTime) {
			$top = $this->chartOriginY + $this->chartY;
			$right = $this->chartOriginX + $this->chartX;
			// Draw top border
			imageline($this->img, $this->chartOriginX, $top, $right, $top, $this->color($borderColor));
			// Erase right border
			imageline($this->img, $right, $top + 1, $right, $top + 26, $this->color($this->bgColor));
			$bars = min(floor($this->chartX / $this->barWidth), $this->dataLength);
			// Guess interval: smallest of the last two, in case the last one
			// happens to be a gap.
			$interval = min(($this->data[$this->dataLength - 1][0] - $this->data[$this->dataLength - 2][0]), ($this->data[$this->dataLength - 2][0] - $this->data[$this->dataLength - 3][0]));
			if ($mGridCol !== false) {
				$G = $mGridCol;
				$T = IMG_COLOR_TRANSPARENT;
				imagesetstyle($this->img, Array($G, $G, $G, $G, $G, $G, $G, $G, $G, $T, $T, $T, $G, $G, $G, $T, $T, $T, $G, $G, $G, $T, $T, $T));
			};
			$lastDate = getdate($this->data[$this->dataLength - 1][0]);
			$lastMajorX = $right;
			$lastMinorX = $right;
			for($j=1, $i=$this->dataLength - 1; $j <= $bars; $i--, $j++) {
				$x = $this->chartOriginX + $this->chartX - ($j * $this->barWidth) - 2;
				$prevDate = getdate($this->data[$i - 1][0]);
				$isMajor = false;
				$isMinor = false;
				$label = '???';
				if ($interval <= 600) {
					// Interval: 10 min or less
					// Major: day changes
					// Minor: hour changes
					if ($lastDate['mday'] != $prevDate['mday']) {
						$isMajor = true;
						$label = date('M j', $this->data[$i][0]);
					} elseif ($lastDate['hours'] != $prevDate['hours']) {
						$isMinor = true;
						$label = date('G:i', $this->data[$i][0]);
					};
				} elseif ($interval <= 7200) {
					// Interval: 10 min to 2 hours
					// Major: month changes
					// Minor: day changes
					if ($lastDate['mon'] != $prevDate['mon']) {
						$isMajor = true;
						$label = date('M', $this->data[$i][0]);
					} elseif ($lastDate['mday'] != $prevDate['mday']) {
						$isMinor = true;
						$label = date('j', $this->data[$i][0]);
					};
				} elseif ($interval <= 86400) {
					// Interval: 2 hours to daily
					// Major: month changes (identify year in label)
					if ($lastDate['mon'] != $prevDate['mon']) {
						$isMajor = true;
						$label = date('M Y', $this->data[$i][0]);
					};
				} else {
					// Interval: multiple days or more
					// Major: year changes
					// Minor: month changes
					if ($lastDate['year'] != $prevDate['year']) {
						$isMajor = true;
						$label = date('Y', $this->data[$i][0]);
					} elseif ($lastDate['mon'] != $prevDate['mon']) {
						$isMinor = true;
						$label = date('M', $this->data[$i][0]);
					};
				};
				$lastDate = $prevDate;
				if ($isMajor) {
					if ($x + (strlen($label) * imagefontwidth(2)) < $lastMajorX) {
						if ($gridCol !== false) imageline($this->img, $x, $this->chartOriginY, $x, $top, IMG_COLOR_STYLED);
						imageline($this->img, $x, $top, $x, $top + 6, $this->color($borderColor));
						imagestring($this->img, 2, $x + 2, $top + 13, $label, $this->color($priceColor));
						$lastMajorX = $x;
					};
				} elseif ($isMinor) {
					if ($x + (strlen($label) * imagefontwidth(2)) < $lastMinorX) {
						if ($gridCol !== false) imageline($this->img, $x, $this->chartOriginY, $x, $top, $gridCol);
						imageline($this->img, $x, $top, $x, $top + 2, $this->color($borderColor));
						imagestring($this->img, 2, $x + 2, $top + 2, $label, $this->color($priceColor));
						$lastMinorX = $x;
					};
				};
			};
		};
	}

	// Method: plotPrice
	// 
	// Plot OHLC price bars. Omitting a down colour draws all bars in the same
	// colour.
	//
	// Parameters:
	//
	// upColor   - 24-bit colour integer
	// downColor - 24-bit colour integer (Optional.)
	//
	// Returns:
	// No return value.
	function plotPrice($upColor, $downColor = false) {
		$upCol = $this->color($upColor);
		$downCol = ($downColor !== false) ? $this->color($downColor) : $upCol;
		$bars = min(floor($this->chartX / $this->barWidth), $this->dataLength);

		for($j=1, $i=$this->dataLength - 1; $j <= $bars; $i--, $j++) {
			$col = ($this->data[$i][1] > $this->data[$i][4]) ? $downCol : $upCol;
			$x = $this->chartOriginX + $this->chartX - ($j * $this->barWidth);
			if ($this->barWidth > 2) imagesetpixel($this->img, $x-1, $this->priceY($this->data[$i][1]), $col);  // Open pixel
			imageline($this->img, $x, $this->priceY($this->data[$i][2]), $x, $this->priceY($this->data[$i][3]), $col);  // High-Low bar
			if ($this->barWidth > 2) imagesetpixel($this->img, $x+1, $this->priceY($this->data[$i][4]), $col);  // Close pixel
		};
	}

	// Method: plotVolume
	// 
	// Create and plot a volume histogram area.
	//
	// Caveats:
	// It is necessary to do this *before* invoking <plotPriceLegends()> and
	// <plotPrice()> because it reduces the height of the OHLC chart area.
	//
	// Parameters:
	// color       - 24-bit colour integer for the histogram.
	// borderColor - 24-bit colour integer for the border.
	// titleColor  - 24-bit colour integer for the inline "Volume" title.
	// height      - Height in pixels.
	function plotVolume($color, $borderColor, $titleColor, $height) {
		$borderCol = $this->color($borderColor);
		$col = $this->color($color);
		$titleCol = $this->color($titleColor);
		$this->chartY -= $height;  // Make room for volume at the bottom
		// Draw borders
		imageline($this->img, $this->chartOriginX, $this->chartOriginY + $this->chartY + 1, $this->x, $this->chartOriginY + $this->chartY + 1, $borderCol);
		imageline($this->img, $this->chartOriginX + $this->chartX, $this->chartOriginY + $this->chartY, $this->chartOriginX + $this->chartX, $this->y, $borderCol);
		// Draw legend
		imagestring($this->img, 2, $this->chartOriginX + $this->chartX + 3, $this->chartOriginY + $this->chartY + 1, number_format($this->maxVol, 0, '.', ','), $titleCol);
		imagestring($this->img, 2, $this->chartOriginX + $this->chartX + 3, $this->y - 13, "0", $titleCol);
		// Draw volume histogram
		$bars = min(floor($this->chartX / $this->barWidth), $this->dataLength);
		for ($j=1, $i=$this->dataLength - 1; $j <= $bars; $i--, $j++) {
			$x = $this->chartOriginX + $this->chartX - ($j * $this->barWidth);
			$y = $this->y - ($this->data[$i][5] / $this->maxVol * ($height - 1)) + 1;
			imagefilledrectangle($this->img, $x-1, $y, $x + $this->barWidth - 3, $this->chartOriginY + $this->y, $col);
		};
		// Draw title
		imagestring($this->img, 2, $this->chartOriginX + 3, $this->chartOriginY + $this->chartY + 1, "Volume", $titleCol);
	}

	// Method: plotVolumeBackground
	// 
	// Plot volume histogram as background of chart area. You want to call this
	// *after* <plotPriceLegends()> but *before* <plotPrice()> for the desired
	// effect.
	//
	// Parameters:
	// color       - 24-bit colour integer for the histogram.
	// height      - Height from the bottom in pixels. (Optional.)
	function plotVolumeBackground($color, $height = false) {
		$col = $this->color($color);
		if ($height === false) $height = $this->chartY;
		$bars = min(floor($this->chartX / $this->barWidth), $this->dataLength);
		for ($j=1, $i=$this->dataLength - 1; $j <= $bars; $i--, $j++) {
			$x = $this->chartOriginX + $this->chartX - ($j * $this->barWidth);
			$y = $this->y - ($this->data[$i][5] / $this->maxVol * ($height - 1)) + 1;
			imagefilledrectangle($this->img, $x-1, $y, $x + $this->barWidth - 3, $this->chartOriginY + $this->y, $col);
		};
	}

	// Method: plotTrend
	//
	// Draw a trend line. The line starts at the origin coordinates, then
	// becomes dashed after the second coordinates.
	//
	// Caveats:
	//
	// The coordinates are relative to the *entire image* not just the price
	// area. This is to help users (such as myself) with external sources of
	// coordinates. (In my case, JavaScript mouse click events.)
	//
	// Parameters:
	// x1 - X of origin pixel
	// y1 - Y of origin pixel
	// x2 - X of second pixel
	// y2 - Y of second pixel
	// color - 24-bit colour integer for the line
	//
	// Returns:
	// No return value.
	function plotTrend($x1, $y1, $x2, $y2, $color) {
		// Don't tolerate vertical lines.
		if ($x1 == $x2) return;
		$brush = imagecreate(3,3);
		imagefill($brush, 0, 0, imagecolorallocate($brush, ($color >> 16) & 0xFF, ($color >> 8) & 0xFF, $color & 0xFF));
		imagesetbrush($this->img, $brush);
		imageline($this->img, $x1, $y1, $x2, $y2, IMG_COLOR_BRUSHED);
		$G = $this->color($color);
		$T = IMG_COLOR_TRANSPARENT;
		imagesetstyle($this->img, Array($T, $T, $T, $T, $T, $T, $G, $G, $G, $G, $G, $G, $G, $G, $G, $G));
		$x3 = $this->chartOriginX + $this->chartX;
		$y3 = $y2;
		if ($y1 != $y2) {
			$y3 = ($y1 == $y2) ? $y1 : floor((($y2 - $y1)/($x2 - $x1))*($x3 - $x1)) + $y1;
			$y4 = $this->chartOriginY + (($y2 - $y1) > 0 ? $this->chartY : 0);
			$x4 = ($x1 == $x2) ? $x1 : floor((($x2 - $x1)/($y2 - $y1))*($y4 - $y1)) + $x1;
			// Display the line which will be shortest
			if (pow($x3 - $x2, 2) + pow($y3 - $y2, 2) > pow($x4 - $x2, 2) + pow($y4 - $y2, 2)) {
				$x3 = $x4;
				$y3 = $y4;
			};
		};
		imageline($this->img, $x2, $y2, $x3, $y3, IMG_COLOR_STYLEDBRUSHED);
		imagedestroy($brush);
	}

	// Method: plotPriceStudy
	//
	// Draws a broken line. The array of data is expected to have exactly as
	// many elements as the bars passed to <ChartOHLC> at creation time. Each
	// value in the array must be in the displayable price range, or it will be
	// omitted.
	//
	// Caveats:
	//
	// The displayable price range will *not* be stretched to accomodate values
	// outside of it, as it was determined in <plotPriceLegends()> which should
	// be called before other plotting methods including this one.
	//
	// Parameters:
	// data  - Indexed array of price values to plot.
	// color - 24-bit colour integer for the broken line.
	// smooth - Smoothing: 0=none, 1=mild, 2=more. (Optional.)
	//
	// Returns:
	// No return value.
	function plotPriceStudy($data, $color, $smooth = 0) {
		$col = $this->color($color);
		$bars = min(floor($this->chartX / $this->barWidth), $this->dataLength);
		$xp = false;
		$yp = false;
		$ypp = false;
		for($j=1, $i=$this->dataLength - $bars; $j <= $bars; $i++, $j++) {
			$y = $this->priceY($data[$i]);
			if ($smooth && $xp && $yp) {
				if ($ypp  &&  $j < $bars - 1  &&  $smooth > 1) $y = round(($ypp + $yp + $y + $this->priceY($data[$i+1]) + $this->priceY($data[$i+2])) / 5);
				elseif ($j < $bars) $y = round(($yp + $y + $this->priceY($data[$i+1])) / 3);
				else $y = round(($yp + $y) / 2);
			};
			if ($y >= $this->chartOriginY  &&  $y <= $this->chartOriginY + $this->chartY) {
				$x = $this->chartOriginX + $this->chartX - (($bars - $j) * $this->barWidth) - 1;
				if ($xp && $yp) {
					// imagesetpixel($this->img, $x, $y, $col);
					imageline($this->img, $xp, $yp, $x, $y, $col);
				};
				$xp = $x;
				$ypp = $yp;
				$yp = $y;
			} else {
				$xp = false;
				$yp = false;
			};
		};
	}

	// Function: plotLevelStudy
	//
	// FIXME
	//
	function plotLevelStudy($levels, $color) {
		foreach ($levels as $level) {
			$y = $this->priceY($level[0]);
			imageline($this->img, $this->chartOriginX, $y, $this->chartOriginX + $this->chartX, $y, $this->color($color, abs($level[1] / 30)));
		};
	}

	// Private Group: Private Methods

	// Private Method: color
	//
	// Resolve a colour integer into a GD colour index. The integer's least
	// significant 24 bits are used as a sequence of 8-bit R, G and B values.
	// (i.e. 0xFFFFFF is white) If the colour was already referenced for the
	// image, its previous index is returned to make absolutely certain that
	// we're using the same palette index for a same colour.
	//
	// If supplied, the intensity between 0 and 1 is compared to the chart
	// background. Note that 0 actually means 25% and 1 means 100% for clarity.
	//
	// Parameters:
	// num   - 24-bit integer colour representation. (i.e. 0xFF0000)
	// alpha - Float 0..1 of intended intensity from chart background.
	//         (Optional.)
	//
	// Returns:
	// The appropriate GD colour index.
	private function color($num, $alpha = null) {
		$r = ($num >> 16) & 0xFF;
		$g = ($num >> 8) & 0xFF;
		$b = $num & 0xFF;
		if ($alpha) {
			if ($alpha < 0) $alpha = 0;
			elseif ($alpha > 1) $alpha = 1;
			$alpha = 0.25 + ($alpha * 0.75);
			$br = ($this->bgColor >> 16) & 0xFF;
			$bg = ($this->bgColor >> 8) & 0xFF;
			$bb = $this->bgColor & 0xFF;
			$r = (($r - $br) * $alpha) + $br;
			$g = (($g - $bg) * $alpha) + $bg;
			$b = (($b - $bb) * $alpha) + $bb;
		};
		if (!$alpha && isset($this->colors[$num])) {
			return $this->colors[$num];
		} else {
			return $this->colors[$num] = imagecolorallocate($this->img, $r, $g, $b);
		};
	}

	// Private Method: priceY
	//
	// Resolve a price into a vertical position.
	//
	// Caveats:
	// *NO LIMITS ARE ENFORCED* so it is entirely possible to obtain a negative
	// or otherwise useless position.
	//
	// Parameters:
	// price - Price to position.
	//
	// Returns:
	// The equivalent vertical position in the OHLC chart window.
	private function priceY($price) {
		return round($this->chartOriginY + $this->chartY - ((($price - $this->low) / $this->range) * $this->chartY));
	}

}

?>

<?php

// Class: YahooFinance
//
// Obtain and parse data from Yahoo! Finance. I'm not sure about the
// legitimacy of this, but I know it's been used widely for 5+ years,
// including as recently as 2008 in HQuote Pro downloader.
class YahooFinance {

	// Section: Introduction

	// Topic: Legal
	//
	// Copyright (C) 2008 Stephane Lavergne <http://www.imars.com/>
	//
	// This program is *proprietary* software. You can *not* redistribute it in
	// any way. This program comes *WITHOUT ANY WARRANTY*; without even the
	// implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

	// Group: Static Methods

	// Method: fetch
	//
	// Download data from the source.
	//
	// Caveats:
	// Since Yahoo! Finance deals with start and end dates, _days_ is
	// multiplied by 7/5 to account for weekends. It is *not* sophisticated
	// enough to account for holidays, so you are not guaranteed to get exactly
	// _days_ bars in the result.
	//
	// Parameters:
	// symbol   - String for the uppercase ticker symbol.
	// interval - One of: daily, weekly, monthly
	// days     - How many days up to the present to request.
	//
	// Returns:
	// The text as it was downloaded, unprocessed.
	static function fetch($symbol, $interval, $days) {
		switch($interval) {
			case 'daily':   $interval='d';  break;
			case 'weekly':  $interval='w';  break;
			case 'monthly': $interval='m';  break;
			default:
				echo "Unknown Yahoo! Finance interval '{$interval}'.\n";
				return '';
				break;
		};
		// CAREFUL: Days are 1-31 but months are 0-11.
		$start = getdate(time() - ($days * 120960));  // 24 * 60 * 60 * 1.4 = 120960
		$sd = $start['mday'];
		$sm = $start['mon'] - 1;
		$sy = $start['year'];
		$end = getdate();
		$ed = $end['mday'];
		$em = $end['mon'] - 1;
		$ey = $end['year'];
		return file_get_contents("http://ichart.finance.yahoo.com/table.csv?s={$symbol}&a={$sm}&b={$sd}&c={$sy}&d={$em}&e={$ed}&f={$ey}&g={$interval}&ignore=.csv");
	}

	// Method: parse
	//
	// Parse text into data array. The input text is expected to be the output
	// of <fetch()>. The two methods have been kept separate to allow
	// applications to save backups of the intermediate data as an added
	// precaution.
	//
	// Parameters:
	// text - The text as returned by <fetch()>.
	//
	// Returns:
	// Indexed array of indexed arrays, each with timestamp int, open, high,
	// low, close doubles and volume int.
	static function parse($text) {
		// Date,Open,High,Low,Close,Volume,Adj Close
		// 2008-08-01,185.00,186.00,178.66,182.00,7495400,182.00
		// CAUTION: Yahoo returns the most recent bar _first_.
		$result = Array();
		$data = explode("\n", $text);
		$max = count($data)-1;
		for ($i=1; $i<$max; $i++) {
			$row = explode(',', $data[$i]);
			array_unshift($result, Array(strtotime($row[0]), $row[1], $row[2], $row[3], $row[4], $row[5]));
		};
		return $result;
	}

}

?>

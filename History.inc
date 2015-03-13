<?php

// Class: History
//
// Store and retrieve historical OHLCV data.
class History {

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
	// This goes hand in hand with <ChartOHLC>.

	// Topic: Usage
	//
	//	Example

	// Undocumented properties
	private $dbh;

	// Group: Database Methods

	// Constructor: History
	//
	// Connect to database. If any table is missing (i.e. first time use), it
	// is created.
	//
	// At this time, tables are: 5min, 15min, 60min, daily, weekly.
	//
	// Parameters:
	// DSN  - PHP PDO database DSN. (Optional.)
	// user - Database user. (Optional.)
	// pass - Database password. (Optional.)
	function History($DSN = 'sqlite:history.sqlite', $user = null, $pass = null) {
		// We do _not_ catch the PDOException if the constructor throws it,
		// because we need for our user to know the connection failed.
		$this->dbh = new PDO($DSN, $user, $pass);
		if ($sth = $this->dbh->prepare("CREATE TABLE IF NOT EXISTS '5min' ( 'ticker' varchar(16), 'time' int, 'open' double, 'high' double, 'low' double, 'close' double, 'volume' int )")) {
			$sth->execute();
		};
		if ($sth = $this->dbh->prepare("CREATE INDEX IF NOT EXISTS '5min_index' ON '5min' ('ticker', 'time')")) {
			$sth->execute();
		};
		if ($sth = $this->dbh->prepare("CREATE TABLE IF NOT EXISTS '15min' ( 'ticker' varchar(16), 'time' int, 'open' double, 'high' double, 'low' double, 'close' double, 'volume' int )")) {
			$sth->execute();
		};
		if ($sth = $this->dbh->prepare("CREATE INDEX IF NOT EXISTS '15min_index' ON '15min' ('ticker', 'time')")) {
			$sth->execute();
		};
		if ($sth = $this->dbh->prepare("CREATE TABLE IF NOT EXISTS '60min' ( 'ticker' varchar(16), 'time' int, 'open' double, 'high' double, 'low' double, 'close' double, 'volume' int )")) {
			$sth->execute();
		};
		if ($sth = $this->dbh->prepare("CREATE INDEX IF NOT EXISTS '60min_index' ON '60min' ('ticker', 'time')")) {
			$sth->execute();
		};
		if ($sth = $this->dbh->prepare("CREATE TABLE IF NOT EXISTS 'daily' ( 'ticker' varchar(16), 'time' int, 'open' double, 'high' double, 'low' double, 'close' double, 'volume' int )")) {
			$sth->execute();
		};
		if ($sth = $this->dbh->prepare("CREATE INDEX IF NOT EXISTS 'daily_index' ON 'daily' ('ticker', 'time')")) {
			$sth->execute();
		};
		if ($sth = $this->dbh->prepare("CREATE TABLE IF NOT EXISTS 'weekly' ( 'ticker' varchar(16), 'time' int, 'open' double, 'high' double, 'low' double, 'close' double, 'volume' int )")) {
			$sth->execute();
		};
		if ($sth = $this->dbh->prepare("CREATE INDEX IF NOT EXISTS 'weekly_index' ON 'weekly' ('ticker', 'time')")) {
			$sth->execute();
		};
	}

	// Destructor: __destruct
	//
	// Disconnect from database.
	function __destruct() {
		$this->dbh = null;
	}

	// Method: get
	//
	// Fetch latest data for a ticker symbol.
	//
	// Parameters:
	// symbol   - Ticker symbol string.
	// interval - One of: 5min, 15min, 60min, daily.
	// count    - How many bars to retrieve.
	//
	// Returns:
	// An indexed array of timestamp, open, high, low, close, volume indexed
	// arrays, *oldest first*. Up to _count_ bars are returned, depending on
	// availability.
	function get($symbol, $interval, $count) {
		if ($sth = $this->dbh->prepare("SELECT MAX(time) FROM '{$interval}' WHERE ticker=?")) {
			if ($sth->execute(Array($symbol))) {
				$result = $sth->fetch();
				if ($result[0]) {
					// CAUTION: We'll always request too much data on Mondays.
					$days = min($days, ceil((time() - $result[0]) / 86400));
					$last = $result[0];
				};
			} else { unset($sth); return; };
			unset($sth);
		} else return;
		$result = Array();
		// To limit to the latest, ORDER BY time DESC and then reverse in PHP.
		if ($sth = $this->dbh->prepare("SELECT time, open, high, low, close, volume FROM '{$interval}' WHERE ticker=? ORDER BY time DESC LIMIT ?")) {
			if ($sth->execute(Array($symbol, $count))) {
				$tmp = $sth->fetchAll(PDO::FETCH_ASSOC);
				foreach ($tmp as $r) {
					array_unshift($result, Array($r['time'], $r['open'], $r['high'], $r['low'], $r['close'], $r['volume']));
				};
			};
		};
		return $result;
	}

	// Group: Maintenance Methods

	// Method: symbols
	//
	// List symbols in daily interval. Useful to get a hint about what's
	// available in the database.
	//
	// Returns:
	// Indexed array of each symbol found.
	function symbols() {
		$result = Array();
		if (($sth = $this->dbh->prepare("SELECT DISTINCT(ticker) AS t FROM 'daily'")) && $sth->execute()) {
			$rows = $sth->fetchAll(PDO::FETCH_ASSOC);
			foreach ($rows as $row) {
				$result[] = $row['t'];
			};
			unset($sth);
		};
		return $result;
	}

	// Method: add
	//
	// Add a symbol and fetch initial history. In order to comply with data
	// source restrictions, the following history lengths are requested: 5min:
	// 5 days, 15min: 10 days, 60min: 20 days, daily: 365 days.
	//
	// Parameters:
	// symbol - Ticker symbol string.
	//
	// Returns:
	// String of detailed explanations on each operation performed.
	function add($symbol) {
		$result = "{$symbol}\n" . str_repeat('=', strlen($symbol)) . "\n\n";
		$result .= "5min: " . $this->updateSymbol($symbol, '5min') . " new bars\n";
		$result .= "15min: " . $this->updateSymbol($symbol, '15min') . " new bars\n";
		$result .= "60min: " . $this->updateSymbol($symbol, '60min') . " new bars\n";
		$result .= "daily: " . $this->updateSymbol($symbol, 'daily') . " new bars\n\n";
		$result .= "weekly: " . $this->updateSymbol($symbol, 'weekly') . " new bars\n\n";
		return $result;
	}

	// Method: update
	//
	// Fetch latest data for all symbols. For each timeframe, each symbol found
	// in the database where the latest tick is older than 12 hours is updated
	// from the appropriate source.
	//
	// Caveat:
	//
	// The data source sites are asked for as much data as seemingly needed to
	// cover the gap between the last tick in the database and the present
	// time. However, the result is *not verified*. All bars with timestamps
	// more recent than the latest found prior to the update are stored. This
	// should suffice if you run daily or at least weekly updates.
	//
	// Returns:
	// String of detailed explanations on each operation performed.
	function update() {
		$result = '';
		foreach(Array('5min', '15min', '60min', 'daily', 'weekly') as $interval) {
			$result .= "INTERVAL: {$interval}\n===============\n\n";
			if (($sth = $this->dbh->prepare("SELECT DISTINCT(ticker) AS t FROM '${interval}'")) && $sth->execute()) {
				$rows = $sth->fetchAll(PDO::FETCH_ASSOC);
				$first = true;
				foreach ($rows as $row) {
					if ($first) {
						$first = false;
					} else {
						sleep(30);
					};
					$added = $this->updateSymbol($row['t'], $interval);
					$result .= "{$row['t']}: {$added} new bars\n";
				};
				unset($sth);
			};
			$result .= "\n";
		};
		return $result;
	}

	// Private Group: Private Methods

	// Private Method: updateSymbol
	//
	// Fetch latest data for a symbol, in one interval. This is called by
	// <update()> and <add()> and does the actual download and storage to DB.
	// If no data is found for the interval, an appropriate duration is fetched
	// as an initial import: 5min: 5 days, 15min: 10 days, 60min: 20 days,
	// daily: 365 days.
	//
	// Note that to avoid overloading remote servers, this sleeps for 2 seconds
	// before actually performing the update.
	//
	// Parameters:
	// symbol   - Ticker symbol string.
	// interval - One of: 5min, 15min, 60min, daily.
	//
	// Returns:
	// Number of records inserted into database.
	function updateSymbol($symbol, $interval) {
		sleep(2);
		$result = 0;
		$days = 2;
		$source = null;
		switch($interval) {
			// ProphetFinance died.  Will need to use Yahoo/Google.
			//case '5min'  :  $days = 5;   $source = 'ProphetFinance';  break;
			//case '15min' :  $days = 10;  $source = 'ProphetFinance';  break;
			//case '60min' :  $days = 20;  $source = 'ProphetFinance';  break;
			case 'daily'  :  $days = 365;  $source = 'YahooFinance';    break;
			case 'weekly' :  $days = 1500; $source = 'YahooFinance';    break;
			default :  return $result;
		};

		// Fetch latest timestamp, if any, for symbol in this interval.
		$last = 0;
		if ($sth = $this->dbh->prepare("SELECT MAX(time) FROM '{$interval}' WHERE ticker=?")) {
			if ($sth->execute(Array($symbol))) {
				$dbResult = $sth->fetch();
				if ($dbResult[0]) {
					// CAUTION: We'll always request too much data on Mondays.
					$days = min($days, ceil((time() - $dbResult[0]) / 86400));
					$last = $dbResult[0];
				};
			};  // No else: there may not be such a ticker in the DB yet.
			unset($sth);
		} else return $result;

		// Download from appropriate source, with backup to "./log/".
		require_once $source.'.inc';
		eval("\$raw = ${source}::fetch(\$symbol, \$interval, \$days);");
		if (is_dir('log')) file_put_contents("log/{$symbol}-{$interval}-".date('Ymd').'.txt', $raw);
		eval("\$cooked = ${source}::parse(\$raw);");

		// Update database.
		if ($sth = $this->dbh->prepare("INSERT INTO '{$interval}' VALUES (?, ?, ?, ?, ?, ?, ?)")) {
			$this->dbh->beginTransaction();
			foreach ($cooked as $row) {
				if ($row[0] > $last) {
					if ($sth->execute(Array($symbol, $row[0], $row[1], $row[2], $row[3], $row[4], $row[5]))) {
						$result++;
					};
				};
			};
			$this->dbh->commit();
		} else return $result;

		return $result;
	}
}

?>

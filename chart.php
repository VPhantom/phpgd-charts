<?php
require_once 'History.inc';
require_once 'ChartOHLC.inc';

$year = date('Y');
$symbol = (isset($_REQUEST['s']) ? $_REQUEST['s'] : 'AAPL');
$interval = (isset($_REQUEST['i']) ? $_REQUEST['i'] : 'daily');
$width = (isset($_REQUEST['w']) ? $_REQUEST['w'] : 600);
$height = (isset($_REQUEST['h']) ? $_REQUEST['h'] : 300);
// $bars = ceil($width / 4) + 50;
// FIXME: this is for debugging Pivots only
$bars = ceil($width / 4);
$theme = (isset($_REQUEST['t']) ? $_REQUEST['t'] : 'black');

// TradeStation black theme
$bgColor = 0x000000;
$gridColor = 0x404040;
$majorGridColor = 0x606060;
$titleColor = 0x00FF00;
$upColor = 0x00FF00;
$downColor = 0xFF0000;
$borderColor = 0xFF0000;
$labelColor = 0xFFFFFF;
$volColor = 0x4040FF;
$lineColor = 0xFF0000;
$bbColor = 0xB0B0B0;
$copyright = "(C) {$year} imars.com";

if ($theme == 'white') {
	// TradeStation research report theme
	$bgColor = 0xFFFFFF;
	$gridColor = 0xE0E0E0;
	$majorGridColor = 0x909090;
	$titleColor = 0x008000;
	$upColor = $titleColor;
	$downColor = false;
	$borderColor = 0xFF6060;
	$labelColor = 0x000000;
	$volColor = 0x6060FF;
	$lineColor = 0xFF6060;
	$copyright .= " but freely redistributable";
};

$hist = new History();
$data = $hist->get($symbol, $interval, $bars);
unset($hist);

$chart = new ChartOHLC($width, $height, $bgColor, $data);
$chart->title("{$symbol} - {$interval} - " . date('Y/m/d', $data[count($data)-1][0]), $titleColor);
$chart->title($copyright, $majorGridColor, true);
$chart->plotVolume($volColor, $borderColor, $labelColor, 40);
$chart->plotPriceLegends($borderColor, $labelColor, $gridColor, true, $majorGridColor);
if ($theme == 'black') {
	require_once 'Studies.inc';

	// Pivots
	// Threshold of 10 removes consolidation stuff to leave the obvious stuff.
	// Increment should be whatever round number is closest to 1% of the
	// low or high price, somewhere around there.  It's clearly 5 for AAPL's
	// 190-325 range; 10 is a bit off and 2.5 is way too verbose.
	// $chart->title(" P(10,5)", 0xFF00FF);
	// $pivots = Studies::Pivots($data, 10, $chart->getLow(), $chart->getHigh(), 5);
	// $chart->plotLevelStudy($pivots, 0xFF00FF);

	// Bollinger Bands
	// $chart->title(" BB(20,2)", $bbColor);
	// $bb = Studies::BollingerBands($data, 20, 2);
	// $chart->plotPriceStudy($bb[0], $bbColor, 2);
	// $chart->plotPriceStudy($bb[2], $bbColor, 2);

	// Moving Average
	$chart->title("MA(10)", 0x2020D0);
	$s = Studies::MA($data, 10);
	$chart->plotPriceStudy($s, 0x2020D0);

	// Moving Average
	$chart->title("MA(20)", 0xC0C000);
	$s = Studies::MA($data, 20);
	$chart->plotPriceStudy($s, 0xC0C000);
};
if (isset($_REQUEST['tl'])) {
	$parts = explode('-', $_REQUEST['tl']);
	list($x1, $y1) = explode('x', $parts[0]);
	list($x2, $y2) = explode('x', $parts[1]);
	$chart->plotTrend($x1, $y1, $x2, $y2, $lineColor);
};
$chart->plotPrice($upColor, $downColor);

header('Content-Type: image/png');
$chart->png();
unset($chart);

?>

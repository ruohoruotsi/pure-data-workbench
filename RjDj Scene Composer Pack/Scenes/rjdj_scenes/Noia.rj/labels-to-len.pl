$prev = 0;
while (<stdin>)
{
	m/([0-9\.]+)/;
	$start_sample = int($_*22050);
	$len = int(1000*($_-$prev));
	$prev = $_;
	print "$len $start_sample;\n";

}

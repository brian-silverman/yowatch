cd "$(dirname "$0")"

TMPDIR=`mktemp -d /tmp/diptrace2oshpark.XXXXXXXXX`

# General Layers
cp BoardOutline.gbr $TMPDIR/yowatch.GKO
cp BottomMask.gbr   $TMPDIR/yowatch.GBS
cp BottomSilk.gbr   $TMPDIR/yowatch.GBO
cp TopMask.gbr      $TMPDIR/yowatch.GTS
cp TopSilk.gbr      $TMPDIR/yowatch.GTO

# Signal layers
cp Top.gbr          $TMPDIR/yowatch.GTL
cp GND.gbr          $TMPDIR/yowatch.GBL
cp Inner.gbr        $TMPDIR/yowatch.G2L
cp VCC.gbr          $TMPDIR/yowatch.G3L

# Drill
cp Through.drl      $TMPDIR/yowatch.XLN

zip -j yowatch.zip $TMPDIR/*

rm -rf $TMPDIR

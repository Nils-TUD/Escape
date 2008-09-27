<?php
for($i = 0;$i < 256;$i++) {
	echo 'intrpt_setIDT('.$i.',isr'.$i.',IDT_DPL_KERNEL);'."\n";
}
?>

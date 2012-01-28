for($i := 0; $i < 4; $i++) do
	time dd if=/dev/zero of=/dev/null count=100000 &
done

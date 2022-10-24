

#include"stdafx.h"

using ff::exec;

int main()
{
	//exec("trackers.test_accuracy");  //test for a single sequence
	
	exec("trackers.test_accuracy_all"); //test for all sequences of RBOT dataset

	
	//exec("trackers.test_on_bcot");
	return 0;
}


 
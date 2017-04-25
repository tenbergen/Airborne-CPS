#include "stdafx.h"
#include "CppUnitTest.h"
#include "component/Decider.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ExampleGaugeTest
{		
	TEST_CLASS(decider_test)
	{
	public:
		
		TEST_METHOD(test_determineActionRequired)
		{
			Aircraft a = Aircraft("MYAIRCRAFT", "12345");
			Decider d = Decider(a, ); //MOCK THE PARAMETERS FOR THIS OUT
			Aircraft a2 = sdnfjknsdjasdsa; //MOCK THIS TOO
			d.Analyze(&a2);
			// CHECK FOR RESULTS WANTED
		}

	};
}
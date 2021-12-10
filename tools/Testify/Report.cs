﻿using System;
using System.Collections.Generic;
using System.Text;
using System.Linq;

namespace Testify
{
    class Report
    {
        private List<TestRun> _tests = new List<TestRun>();

        public void AddTest(TestRun info)
        {
            _tests.Add(info);
        }

        /// <summary>
        /// Overall testsuite runtime in milliseconds
        /// </summary>
        public long OverallRuntime { get; set; } = 0;

        public IEnumerable<TestRun> Tests {  get { return _tests; } }

        /// <summary>
        /// Might be because we have -fail-fast set and a test failed.
        /// </summary>
        public bool AbortedEarly = false;

        public int GetTotalWithStatus(TestStatus status)
        {
            return _tests.Count((TestRun run) => { return run.Status == status; });
        }

        public int TotalTestsRan { get { return _tests.Count; } }

        public long GetSmallestCompilationTime()
        {
            return _tests.Min((TestRun run) => { return run.CompilationTime; });
        }

        public long GetLargestCompilationTime()
        {
            return _tests.Max((TestRun run) => { return run.CompilationTime; });
        }

        public double GetAverageCompilationTime()
        {
            return _tests.Average((TestRun run) => { return run.CompilationTime; });
        }
    }
}

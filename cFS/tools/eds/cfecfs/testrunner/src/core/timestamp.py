'''
Copyright (c) 2025 United States Government as represented by
the Administrator of the National Aeronautics and Space Administration.
All Rights Reserved.

'''
import time
import operator

class Timestamp:

    # by default use the monotonic clock for all computations
    REF = time.CLOCK_MONOTONIC
    NONE = None

    @staticmethod
    def now():
        return Timestamp(time.clock_gettime_ns(Timestamp.REF))

    @classmethod
    def undefined(self):
        if self.NONE is None:
            self.NONE = Timestamp()
            self.NONE.ticks = None

        return self.NONE

    @classmethod
    def from_ticks(self,ticks):
        if ticks is None:
            result = self.undefined()
        else:
            result = self(ticks)
        return result

    @classmethod
    def from_relative_ticks(self,relative_ticks):
        result = self.from_ticks(relative_ticks)
        if result.ticks:
            result.ticks += time.clock_gettime_ns(Timestamp.REF)
        return result

    @classmethod
    def from_relative_seconds(self,relative_seconds):
        return self.from_relative_ticks(relative_seconds * 1000000000)

    def calc_ticks(self, other, op):
        if (self.ticks is not None and other is not None):
            result = op(self.ticks, int(other))
        else:
            result = None # operation with None is always undefined (like NaN)

        return result

    def remaining_seconds(self):
        return float(self.calc_ticks(Timestamp.now(), operator.sub)) / 1000000000

    def __int__(self):
        return int(self.ticks)

    def __add__(self, other):
        return Timestamp.from_ticks(self.calc_ticks(other, operator.add))

    def __sub__(self, other):
        return Timestamp.from_ticks(self.calc_ticks(other, operator.sub))

    def __iadd__(self, other):
        new_ticks = self.calc_ticks(other, operator.add)
        if new_ticks is None:
            raise ValueError("Undefined result")
        self.ticks = new_ticks

    def __isub__(self, other):
        new_ticks = self.calc_ticks(other, operator.sub)
        if new_ticks is None:
            raise ValueError("Undefined result")
        self.ticks = new_ticks

    def __bool__(self):
        return self.ticks is not None

    def __eq__(self, other):
        return self.calc_ticks(other, operator.eq)

    def __ne__(self, other):
        return self.calc_ticks(other, operator.ne)

    def __gt__(self, other):
        return self.calc_ticks(other, operator.gt)

    def __lt__(self, other):
        return self.calc_ticks(other, operator.lt)

    def __ge__(self, other):
        return self.calc_ticks(other, operator.ge)

    def __le__(self, other):
        return self.calc_ticks(other, operator.le)

    def __init__(self, ticks=0):
        if ticks is None:
            raise ValueError("Attempt to create duplicate undefined timestamp.  Use Timestamp.from_ticks() when value might be None.")
        self.ticks = int(ticks)

 --- @file    expectation.lua
 --- @author  Paul Thomas
 --- @date    4/3/2023
 --- @brief
 ---
 --- @copyright
 --- This program is free software: you can redistribute it and/or modify it under the
 --- terms of the GNU General Public License as published by the Free Software Foundation, either
 --- version 3 of the License, or (at your option) any later version.
 --- This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
 --- even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 --- General Public License for more details.
 --- You should have received a copy of the GNU General Public License along with this program. If
 --- not, see <a href=https://www.gnu.org/licenses/>https://www.gnu.org/licenses/</a>.

 Sequence("Expectation", function()
     Test("T1", function()
         Expect(true, "ToBeTrue"):ToBeTrue()
     end)

     Test("T2", function()
         Requires(Test("T1"):ToPass())
         RequirementSpecifier(function() return true end)
     end)
 end)

 Sequence("S", function()
     Test(">", function()
         Expect(-1, "A"):Not():ToBeGreater(0)
         Expect(0, "B"):Not():ToBeGreater(0)
         Expect(1, "C"):ToBeGreater(0)
     end)

     Test(">=", function()
         Expect(-1, "A"):Not():ToBeGreaterOrEqual(0)
         Expect(0, "B"):ToBeGreaterOrEqual(0)
         Expect(1, "C"):ToBeGreaterOrEqual(0)
     end)

     Test("<", function()
         Expect(-1, "A"):ToBeLesser(0)
         Expect(0, "B"):Not():ToBeLesser(0)
         Expect(1, "C"):Not():ToBeLesser(0)
     end)

     Test("<=", function()
         Expect(-1, "A"):ToBeLesserOrEqual(0)
         Expect(0, "B"):ToBeLesserOrEqual(0)
         Expect(1, "C"):Not():ToBeLesserOrEqual(0)
     end)
 end)
 
 Sequence("Show", function()
    Test("Shown", function()
        Expect(true, "ToBeTrue - Shown"):ToBeTrue():Show()
        Expect(false, "ToBeFalse - Shown"):ToBeFalse():Show()
        Expect(3.1415, "ToBeGreater - Shown"):ToBeGreater(3):Show()
        Expect(3.1415, "ToBeGreaterOrEqual - Shown"):ToBeGreaterOrEqual(3):Show()
        Expect(3.1415, "ToBeLesser - Shown"):ToBeLesser(4):Show()
        Expect(3.1415, "ToBeLesserOrEqual - Shown"):ToBeLesserOrEqual(4):Show()
        Expect(3.1415, "ToBeNear - Shown"):ToBeNear(3, 0.2):Show()
        Expect(3.1415, "ToBeInRange - Shown"):ToBeInRange(3, 4):Show()
        Expect(3.1415, "ToBeInPercentage - Shown"):ToBeInPercentage(3, 10):Show()
        Expect(42, "ToBeEqual - Shown"):ToBeEqual(42):Show()
    end)
    Test("Not Shown", function()
        Expect(true, "ToBeTrue - Not Shown"):ToBeTrue()
        Expect(false, "ToBeFalse - Not Shown"):ToBeFalse()
        Expect(3.1415, "ToBeGreater - Not Shown"):ToBeGreater(3)
        Expect(3.1415, "ToBeGreaterOrEqual - Not Shown"):ToBeGreaterOrEqual(3)
        Expect(3.1415, "ToBeLesser - Not Shown"):ToBeLesser(4)
        Expect(3.1415, "ToBeLesserOrEqual - Not Shown"):ToBeLesserOrEqual(4)
        Expect(3.1415, "ToBeNear - Not Shown"):ToBeNear(3, 0.2)
        Expect(3.1415, "ToBeInRange - Not Shown"):ToBeInRange(3, 4)
        Expect(3.1415, "ToBeInPercentage - Not Shown"):ToBeInPercentage(3, 10)
        Expect(42, "ToBeEqual - Not Shown"):ToBeEqual(42)
    end)
 end)

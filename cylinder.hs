import Data.List
import System.IO

vGen:: Float -> Float -> Int -> Int -> [String]
vGen _ _ 0 _ = []
vGen r z it res = [line] ++ [line] ++ vGen r z (it-1) res
                    where line = "v " ++ show x ++ " " ++ show y ++ " " ++ show z ++ " "
                          x = r*cos((2.0*pi/a) * (a - b))
                          y = r*sin((2.0*pi/a) * (a - b))
                          a = fromIntegral res :: Float
                          b = fromIntegral it :: Float

vnGen:: Bool -> Int -> Int -> [String]
vnGen _ 0 _ = []
vnGen up it res = ["vn " ++ vCords] ++ ["vn " ++ hCords] ++ vnGen up (it-1) res
                  where vCords = if up then "0.0 0.0 1.0" else "0.0 0.0 -1.0"
                        hCords = show (cos((2.0*pi/a) * (a - b))) ++ " " ++ show (sin((2.0*pi/a) * (a - b))) ++ " 0.0"
                        a = fromIntegral res :: Float
                        b = fromIntegral it :: Float

topTria:: Int -> Int -> [String]
topTria it res = if it < res - 1 then ["f 1//1 " ++ a ++ b] ++ topTria (it+1) res else ["f 1//1 " ++ a ++ b]
                 where a = show (it*2 - 1) ++ "//" ++ show (it*2 - 1) ++ " "
                       b = show (it*2 + 1) ++ "//" ++ show (it*2 + 1) ++ " "

botTria:: Int -> Int -> [String]
botTria it res = if it < res - 1 then [c ++ a ++ b] ++ botTria (it+1) res else [c ++ a ++ b]
                 where a = show (d + it*2 - 1) ++ "//" ++ show (d + it*2 - 1) ++ " "
                       b = show (d + it*2 + 1) ++ "//" ++ show (d + it*2 + 1) ++ " "
                       c = "f " ++ show (d + 1) ++ "//" ++ show (d + 1) ++ " "
                       d = res * 2

botSideTria:: Int -> Int -> [String]
botSideTria it res = if it < res then ["f " ++ a ++ b ++ c] ++ botSideTria (it+1) res 
                     else ["f " ++ a ++ b ++ show (res * 2 + 2) ++ "//" ++ show (res * 2 + 2)]
                     where a = show (2*it) ++ "//" ++ show (2*it) ++ " "
                           b = show(d + 2*it) ++ "//" ++ show(d + 2*it) ++ " "
                           c = show(d + 2*it + 2) ++ "//" ++ show(d + 2*it + 2) ++ " "
                           d = res * 2

topSideTria:: Int -> Int -> [String]
topSideTria it res = if it < res then ["f " ++ c ++ b ++ a] ++ topSideTria (it+1) res 
                     else ["f " ++ show(d + 2) ++ "//" ++ show(d + 2) ++ " 2//2 " ++ a]
                     where a = show(2*it) ++ "//" ++ show(2*it) ++ " "
                           b = show(2*it + 2) ++ "//" ++ show(2*it + 2) ++ " "
                           c = show(d + it*2 + 2) ++ "//" ++ show(d + it*2 + 2) ++ " "
                           d = res * 2

fGen:: Int -> [String]
fGen res = topTria 2 res ++ botTria 2 res ++ topSideTria 1 res ++ botSideTria 1 res

cylGen:: Float -> Float -> Int -> [String]
cylGen r h res = (vGen r (h/2) res res) ++ (vGen r (-h/2) res res) ++ (vnGen True res res) ++ (vnGen False res res) ++ fGen res

main = writeFile "/cyl.obj" (intercalate "\n" (cylGen 1.0 2.0 32))

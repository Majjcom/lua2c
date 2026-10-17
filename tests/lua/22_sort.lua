-- 合并两个已排序的数组
local function merge(left, right)
    local result = {}
    local leftIndex = 1
    local rightIndex = 1

    -- 比较两个数组的头部元素，将较小的放入结果数组
    while leftIndex <= #left and rightIndex <= #right do
        if left[leftIndex] <= right[rightIndex] then
            table.insert(result, left[leftIndex])
            leftIndex = leftIndex + 1
        else
            table.insert(result, right[rightIndex])
            rightIndex = rightIndex + 1
        end
    end

    -- 将剩余的元素追加到结果数组后面
    while leftIndex <= #left do
        table.insert(result, left[leftIndex])
        leftIndex = leftIndex + 1
    end

    while rightIndex <= #right do
        table.insert(result, right[rightIndex])
        rightIndex = rightIndex + 1
    end

    return result
end

-- 归并排序主函数
function mergeSort(arr)
    -- 如果数组只有一个元素或为空，则无需排序
    if #arr <= 1 then
        return arr
    end

    -- 分割步骤：找到中间位置
    local mid = math.floor(#arr / 2)
    local leftPart = {}
    local rightPart = {}

    -- 将数组分为左右两部分
    for i = 1, mid do
        table.insert(leftPart, arr[i])
    end
    for i = mid + 1, #arr do
        table.insert(rightPart, arr[i])
    end

    -- 递归排序左右两部分
    local leftSorted = mergeSort(leftPart)
    local rightSorted = mergeSort(rightPart)

    -- 合并排序后的两部分
    return merge(leftSorted, rightSorted)
end

-- 测试示例
math.randomseed(4628743)
print("Generating...")
local testArray = {}
--[[12, 23, 4, 56, 78, 9, 32, 1, 45, 67, 89, 3]]
for _ = 1, 100 do
    table.insert(testArray, math.floor(math.random(1, 500000)))
end

local start = os.clock()
-- 归并排序会返回一个新数组，不会修改原数组
local sortedArray = mergeSort(testArray)
local t = os.clock() - start

print("After sort:")
for i = 1, #sortedArray do
    io.write(sortedArray[i] .. " ")
end
--print("Time used:", t)
print()
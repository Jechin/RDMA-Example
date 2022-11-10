'''
Description: 状态更新函数推演
Autor: Jechin
Date: 2022-11-09 16:21:02
LastEditors: Jechin
LastEditTime: 2022-11-10 10:24:55
'''

from tqdm import tqdm
import os

def print_list(var, list):
    with open(f"./VarExpression/{var}.txt", "w") as f:
        for item in list:
            f.write(item)
    print("written in " + var + ".txt")

def SS1(i):
    ans = ["(", "(", "(", f"A_{i}", "<<<12", ")", "+", f"E_{i}", "+", f"(T_{i}<<<{i})", ")", "<<<7", ")"]
    return ans

def extend(list1, list2, index):
    if index > len(list1):
        print("index out of range, extend error")
        exit(0)
    for i in range(len(list2)):
        list1.insert(index + i, list2[i])

    return list1


def SS2(i):
    ans = []
    ans.append("(")
    ans.append(f"SS1_{i}")
    ans.append("+")
    ans.append("(")
    ans.append(f"A_{i}")
    ans.append("<<<12")
    ans.append(")")
    ans.append(")")
    return ans

def TT1(i):
    ans = []
    ans.append("(")
    FF = []
    if i < 16:
        FF += ["(", f"A_{i}", "^", f"B_{i}", "^", f"C_{i}", ")"]
    elif i < 64:
        FF += ["(", "(", f"A_{i}", "&", f"B_{i}", ")", "|", "(", f"A_{i}", "&", f"C_{i}", ")", "|", "(", f"B_{i}", "&", f"C_{i}", ")", ")"]
    else:
        print("TT1 error")
        exit(0)
    ans += FF
    ans += ["+", f"D_{i}", "+", f"SS2_{i}", "+", f"W'_{i}"]

    ans.append(")")
    return ans

def TT2(i):
    ans = []
    ans.append("(")
    GG = []
    if i < 16:
        GG += ["(", f"E_{i}", "^", f"F_{i}", "^", f"G_{i}", ")"]
    elif i < 64:
        GG += ["(", "(", f"E_{i}", "&", f"F_{i}", ")", "|", "(", "~", f"E_{i}", "&", f"G_{i}", ")", ")"]
    else:
        print("TT2 error")
        exit(0)
    ans += GG
    ans += ["+", f"H_{i}", "+", f"SS1_{i}", "+", f"W_{i}"]
    ans.append(")")
    return ans

def D(i):
    if i % 8 == 0:
        return [f"D_{i}"]
    elif os.path.exists(f"./varExpression/D_{i}.txt"):
        with open(f"./varExpression/D_{i}.txt", "r") as f:
            ans = f.read()
        return [ans]
    else:
        return ["(", f"C_{i-1}", ")"]

def C(i):
    if i % 8 == 0:
        return [f"C_{i}"]
    elif os.path.exists(f"./varExpression/C_{i}.txt"):
        with open(f"./varExpression/C_{i}.txt", "r") as f:
            ans = f.read()
        return [ans]
    else:
        return ["(", f"B_{i-1}", "<<<9", ")"]


def B(i):
    if i % 8 == 0:
        return [f"B_{i}"]
    elif os.path.exists(f"./varExpression/B_{i}.txt"):
        with open(f"./varExpression/B_{i}.txt", "r") as f:
            ans = f.read()
        return [ans]
    else:
        return ["(", f"A_{i-1}", ")"]


def A(i):
    if i % 8 == 0:
        return [f"A_{i}"]
    elif os.path.exists(f"./varExpression/A_{i}.txt"):
        with open(f"./varExpression/A_{i}.txt", "r") as f:
            ans = f.read()
        return [ans]
    else:
        return ["(", f"TT1_{i-1}", ")"]

def H(i):
    if i % 8 == 0:
        return [f"H_{i}"]
    elif os.path.exists(f"./varExpression/H_{i}.txt"):
        with open(f"./varExpression/H_{i}.txt", "r") as f:
            ans = f.read()
        return [ans]
    else:
        return ["(", f"G_{i-1}", ")"]

def G(i):
    if i % 8 == 0:
        return [f"G_{i}"]
    elif os.path.exists(f"./varExpression/G_{i}.txt"):
        with open(f"./varExpression/G_{i}.txt", "r") as f:
            ans = f.read()
        return [ans]
    else:
        return ["(", f"F_{i-1}", "<<<19", ")"]

def F(i):
    if i % 8 == 0:
        return [f"F_{i}"]
    elif os.path.exists(f"./varExpression/F_{i}.txt"):
        with open(f"./varExpression/F_{i}.txt", "r") as f:
            ans = f.read()
        return [ans]
    else:
        return ["(", f"E_{i-1}", ")"]

def E(i):
    if i % 8 == 0:
        return [f"E_{i}"]
    elif os.path.exists(f"./varExpression/E_{i}.txt"):
        with open(f"./varExpression/E_{i}.txt", "r") as f:
            ans = f.read()
        return [ans]
    else:
        return ["(", f"TT2_{i-1}", "^", "(", f"TT2_{i-1}", "<<<9", ")", "^", "(", f"TT2_{i-1}", "<<<17", ")", ")"]

def Calculate(test):
    iter = 0
    round = 0
    while iter < len(test):
        item = test[iter]
        if item[0:4] == "SS1_":
            SS1_result = SS1(int(item[4:]))
            test.remove(item)
            extend(test, SS1_result, iter)
            round += 1
        elif item[0:4] == "SS2_":
            SS2_result = SS2(int(item[4:]))
            test.remove(item)
            extend(test, SS2_result, iter)
            round += 1
        elif item[0:4] == "TT1_":
            TT1_result = TT1(int(item[4:]))
            test.remove(item)
            extend(test, TT1_result, iter)
            round += 1
        elif item[0:4] == "TT2_":
            TT2_result = TT2(int(item[4:]))
            test.remove(item)
            extend(test, TT2_result, iter)
            round += 1
        elif item[0:2] == "D_":
            if int(item[2:]) == 0:
                iter += 1
            else:
                D_result = D(int(item[2:]))
                test.remove(item)
                extend(test, D_result, iter)
                round += 1
        elif item[0:2] == "C_":
            if int(item[2:]) == 0:
                iter += 1
            else:
                C_result = C(int(item[2:]))
                test.remove(item)
                extend(test, C_result, iter)
                round += 1
        elif item[0:2] == "B_":
            if int(item[2:]) == 0:
                iter += 1
            else:
                B_result = B(int(item[2:]))
                test.remove(item)
                extend(test, B_result, iter)
                round += 1
        elif item[0:2] == "A_":
            if int(item[2:]) == 0:
                iter += 1
            else:
                A_result = A(int(item[2:]))
                test.remove(item)
                extend(test, A_result, iter)
                round += 1
        elif item[0:2] == "H_":
            if int(item[2:]) == 0:
                iter += 1
            else:
                H_result = H(int(item[2:]))
                test.remove(item)
                extend(test, H_result, iter)
                round += 1
        elif item[0:2] == "G_":
            if int(item[2:]) == 0:
                iter += 1
            else:
                G_result = G(int(item[2:]))
                test.remove(item)
                extend(test, G_result, iter)
                round += 1
        elif item[0:2] == "F_":
            if int(item[2:]) == 0:
                iter += 1
            else:
                F_result = F(int(item[2:]))
                test.remove(item)
                extend(test, F_result, iter)
                round += 1
        elif item[0:2] == "E_":
            if int(item[2:]) == 0:
                iter += 1
            else:
                E_result = E(int(item[2:]))
                test.remove(item)
                extend(test, E_result, iter)
                round += 1
        else:
            iter += 1

    return test, round

if __name__ == "__main__":
    # var_list = ["A_1", "B_1", "C_1", "D_1", "E_1", "F_1", "G_1", "H_1"]
    # var_list = ["A_2", "B_2", "C_2", "D_2", "E_2", "F_2", "G_2", "H_2"]
    # var_list = ["A_3", "B_3", "C_3", "D_3", "E_3", "F_3", "G_3", "H_3"]
    # var_list = ["A_4", "B_4", "C_4", "D_4", "E_4", "F_4", "G_4", "H_4"]
    # var_list = ["A_5", "B_5", "C_5", "D_5", "E_5", "F_5", "G_5", "H_5"]
    # var_list = ["A_6", "B_6", "C_6", "D_6", "E_6", "F_6", "G_6", "H_6"]
    # var_list = ["A_7", "B_7", "C_7", "D_7", "E_7", "F_7", "G_7", "H_7"]
    for round in range(8):
        var_list = [f"A_{round}", f"B_{round}", f"C_{round}", f"D_{round}", f"E_{round}", f"F_{round}", f"G_{round}", f"H_{round}"]
        for var in tqdm(var_list):
            test = [var]
            result, _ = Calculate(test)
            print_list(var, result)

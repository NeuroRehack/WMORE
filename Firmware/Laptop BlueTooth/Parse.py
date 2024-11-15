def parser(string) -> tuple:
    x_s, y_s, z_s = string.split(',')[2:5]
    print(string.split(',')[2:5])
    print(f"X: {float(x_s)} | Y: {float(y_s)} | Z: {float(z_s)}")
    
    return tuple(x_s, y_s, z_s)


def parse_time(string) -> str:
    date_s, time_s = string.split(',')[0:2]
    # print(f"Date: {date_s} | Time: {time_s}")
    return date_s + " " + time_s


if __name__ == "__main__":
    parser("01/01/2000,01:23:48.29,66.89,-21.00,-1022.95,3.64,-2.12,-1.12,-3.15,29.70,-5.40,31.72,")
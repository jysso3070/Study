from bs4 import BeautifulSoup
import urllib.request as req
import urllib.parse

def LoadData():
    # Date = None
    # saveName = "ss.xml"
    # url = "http://api.data.go.kr/openapi/tn_pubr_public_clns_shunt_fclty_api?"
    # key = "{ quote_plus('ServiceKey') : IV3K3dvqL%2F0KdhY2pq3ozKbmQ442QEMqeCR45TlWoSFRSKe9oxHwAdlipLl1pM2KtrmGWpVSHslqhpHE6Kxwlg%3D%3D}"
    # url = url + "ServiceKey=" + key #+ "&type=xml&pageNo=1&numOfRows=10&flag=Y"
    # data = urllib.request.urlopen(url).read()
    # text = data.decode('utf-8')
    #
    # req.urlretrieve(url, saveName)

    xml = open("shelter2.xml", "r", encoding="UTF-8").read()
    Data = BeautifulSoup(xml, "html.parser")
    return Data

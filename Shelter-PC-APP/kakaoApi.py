import requests

def kakaoApiAdressSearch(searchstr):

    headers = {
        'Content-Type': 'application/json; charset=utf-8',
        'Authorization': 'KakaoAK cc7aa32645d56c0b897ffd2f28a2da31'
    }
    params = {
        'query': searchstr
    }
    req = requests.get("https://dapi.kakao.com/v2/local/search/address.json", headers=headers, params=params)
    result = req.json()
    return result

def kakaoApiKeywordSearch(searchstr):
    headers = {
        'Content-Type': 'application/json; charset=utf-8',
        'Authorization': 'KakaoAK cc7aa32645d56c0b897ffd2f28a2da31'
    }
    params = {
        'query': searchstr
    }
    req = requests.get("https://dapi.kakao.com/v2/local/search/keyword.json",headers=headers, params=params)
    result = req.json()
    return result


# result = kakaoApiAdressSearch("인천광역시 남동구 논현동 소래휴먼시아3단지")
# result = kakaoApiKeywordSearch("서울특별시 종로구 신문로2가 89번지    ")
#
# print(result)
# print(result["documents"][0]["y"])
# print(result["documents"][0]["x"])
package main;

import (
    "fmt"
    "os"
    "strings"
    "net/http"
    "io/ioutil"
    "bufio"
)

type Distro struct {
    url string;
    architecture string;
};

func Check(err error) {
    if err != nil {
        fmt.Printf("Error: %s\n", err);
        Exit(1);
    }
}

func Exit(value int)  {
    os.Exit(value);
}

func UrlDataGet(url string) string {
    req, err := http.Get(url);
    Check(err);
    defer req.Body.Close();

    if req.StatusCode != http.StatusOK {
        return "";
    }

    bytes, err := ioutil.ReadAll(req.Body);

    return string(bytes);
}


func ParseForLatestDebian(arch string) string {
    base_url := "http://cdimage.debian.org/debian-cd/current/multi-arch/iso-cd/";

    data := UrlDataGet(base_url)

    tag_start := "<a href=\"";
    tag_full := tag_start + "debian-"
    idx := strings.Index(data, tag_full);

    str := data[idx + len(tag_start) :len(data)];
    end := strings.Index(str, "\""); 
    filename := str[0:end]
    return fmt.Sprintf("%s%s", base_url, filename);
}

func ParseForLatestFreeBSD(arch string) string {

    base_url := "http://ftp.freebsd.org/pub/FreeBSD/releases/ISO-IMAGES/";

    data := UrlDataGet(base_url);

    var found string = "";
    var version string = ""

    for i := 11; i < 20; i++ {
        for j := 0; j < 10; j++ {
            version = fmt.Sprintf("%d.%d", i, j);
            if strings.Contains(data, version) {
                found = version;
            }
        }
    }

    if found != "" {
        return fmt.Sprintf("%s%s/FreeBSD-%s-RELEASE-%s-memstick.img", base_url, found, found, arch);
    }
    return version;
}

func ParseForLatestOpenBSD(arch string) string {
    base_url := "http://mirror.ox.ac.uk/pub/OpenBSD/";
 
    data := UrlDataGet(base_url); 

    var found string =  "";
    var version string = "";

    for i := 6; i < 10; i++ {
        for j := 0; j < 10; j++ {
            tmp := fmt.Sprintf("%d.%d", i, j);
            if (strings.Contains(data, tmp)) {
                found = fmt.Sprintf("%d%d", i, j);
                version = tmp;
            }
        }
    }

    if found != "" {
        return fmt.Sprintf("%s%s/%s/install%s.fs", base_url, version, arch, found);
    }

    return "";
}

func LatestReleaseURL(os string, arch string) Distro {
    var url string = "";

    switch (os) {
        case "OpenBSD":
        url = ParseForLatestOpenBSD(arch);
        break;

        case "FreeBSD":
        url = ParseForLatestFreeBSD(arch);
        break;

        case "Debian":
        url = ParseForLatestDebian(arch);
        break;
    }

    if len(url) == 0 {
        fmt.Printf("Error: LatestReleaseURL %s %s\n", os, arch);
        Exit(1);
    }

    latest := Distro { architecture: arch, url: url };

    return latest;
}

func distroGetAll() map[string]Distro {

    distros := make(map[string]Distro);

    distros["Debian GNU/Linux (i386/amd64)"] = LatestReleaseURL("Debian", "amd64");

    distros["OpenBSD (amd64)"] = LatestReleaseURL("OpenBSD", "amd64");
    distros["OpenBSD (i386)"]  = LatestReleaseURL("OpenBSD", "i386");  

    distros["FreeBSD (i386)"]  = LatestReleaseURL("FreeBSD", "i386");
    distros["FreeBSD (amd64)"] = LatestReleaseURL("FreeBSD", "amd64");

    return distros;
}

func main() {

    distros := distroGetAll();

    f, err := os.Create("list.txt");
    Check(err);
    defer f.Close();

    w := bufio.NewWriter(f);

    for name, distro := range distros {
        fmt.Fprintf(w, "%s=%s\n", name, distro.url);
        fmt.Printf("Name: %s and URL %s\n", name, distro.url);
    }

    w.Flush();

    Exit(0);
}
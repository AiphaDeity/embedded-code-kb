import { readFile, readdir } from 'node:fs/promises'
import { join, posix, sep } from 'node:path'

export async function checkSourceDownloads(srcDir: string, outDir: string, base: string) {
  const site = new URL(base, 'https://kb.invalid')
  const pages = (await readdir(outDir, { recursive: true }))
    .map(path => path.split(sep).join('/'))
    .filter(path => path.endsWith('.html') && !path.startsWith('examples/'))
  let checked = 0

  for (const page of pages) {
    const html = await readFile(join(outDir, page), 'utf8')
    const pageUrl = new URL(page, site)
    for (const [tag] of html.matchAll(/<a\s+(?:"[^"]*"|'[^']*'|[^"'<>])*>/gi)) {
      const attributes = new Map<string, string>(Array.from(
        tag.matchAll(/\s+([^\s=/>]+)(?:\s*=\s*(?:"([^"]*)"|'([^']*)'|([^\s>]+)))?/g),
        ([, name, double, single, unquoted]) => [name.toLowerCase(), double ?? single ?? unquoted ?? ''],
      ))
      if (!attributes.has('download')) continue
      const href = attributes.get('href')
      if (!href) throw new Error(`${page} 中的下载链接缺少 href`)
      const target = new URL(href, pageUrl)
      if (target.origin !== site.origin) continue
      const path = decodeURIComponent(target.pathname)
      if (!path.startsWith(site.pathname + 'examples/') || posix.normalize(path) !== path || path.includes('\\')) {
        throw new Error(`${page} 的源码下载链接未指向站内 examples：${href}`)
      }
      const relative = path.slice(site.pathname.length)
      // 文件路径以仓库根为起点。拷贝后复读两个位置，确保发布的内容与示例源码一致。
      const [original, published] = await Promise.all([
        readFile(join(srcDir, relative)),
        readFile(join(outDir, relative)),
      ]).catch(cause => {
        throw new Error(`${page} 的源码下载缺少文件：${href}`, { cause })
      })
      if (!original.equals(published)) throw new Error(`${page} 的源码下载与源文件不一致：${href}`)
      checked++
    }
  }
  console.log(`已核对 ${checked} 个源码下载链接`)
}

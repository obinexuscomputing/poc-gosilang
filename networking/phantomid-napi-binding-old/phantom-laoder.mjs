// phantom-loader.mjs
export async function load(url, context, defaultLoad) {
    if (url.endsWith('.node')) {
      const module = await defaultLoad(url, { ...context, format: 'node' });
      return {
        ...module,
        format: 'module'
      };
    }
    return defaultLoad(url, context);
  }